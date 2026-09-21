// Fill out your copyright notice in the Description page of Project Settings.

#include "Player/FPSCharacter.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Cannons/Projectile.h"
#include "DrawDebugHelpers.h" // debug visualization
#include "Components/PrimitiveComponent.h" // for UPrimitiveComponent
#include "Cannons/Cannon.h" // Include Cannon header
#include "InputCoreTypes.h" // for EKeys

// Sets default values
AFPSCharacter::AFPSCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Ensure controller rotation affects the pawn (so AddControllerPitchInput/Yaw show up)
	bUseControllerRotationPitch = true;
	bUseControllerRotationYaw = true;
	bUseControllerRotationRoll = false;

	// Character movement should not orient to movement; let controller rotation drive view
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->bOrientRotationToMovement = false;
	}

	HeroMeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("HeroMeshComponent")); // Add Component in Unity
	HeroMeshComponent->bCastDynamicShadow = false;
	HeroMeshComponent->CastShadow = false;

	// Create spring arm (camera boom) and attach to capsule so it follows controller rotation
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetCapsuleComponent());
	CameraBoom->TargetArmLength = 0.0f; // keep camera at the pawn location for FPS
	CameraBoom->bUsePawnControlRotation = true; // rotate arm based on controller

	FPSCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FPSCamera"));
	// Attach camera to spring arm so it inherits controller rotation
	FPSCameraComponent->SetupAttachment(CameraBoom);
	// Camera itself should not use pawn control rotation when attached to a rotating spring arm
	FPSCameraComponent->bUsePawnControlRotation = false;

	PhysicsHandle = CreateDefaultSubobject<UPhysicsHandleComponent>(TEXT("PhysicsHandle"));

	HeldProjectile = nullptr;
	HeldComponent = nullptr;
	HeldCannon = nullptr;
}

// Called when the game starts or when spawned
void AFPSCharacter::BeginPlay()
{
	Super::BeginPlay();
	if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))

	{

		if (PlayerMappingContext)

		{

			UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer());

			if (Subsystem)

			{

				Subsystem->AddMappingContext(PlayerMappingContext, 0);

			}

		}

	}
	
}

// Called every frame
void AFPSCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Update held object position every frame (projectiles use PhysicsHandle; cannons are attached to camera)
	if (PhysicsHandle && PhysicsHandle->GrabbedComponent)
	{
		FVector HoldLocation = FPSCameraComponent->GetComponentLocation()
			+ FPSCameraComponent->GetForwardVector() * HoldDistance;
		PhysicsHandle->SetTargetLocation(HoldLocation);

		// Keep angular velocity zero and align rotation to camera so it doesn't spin
		if (HeldComponent && !HeldCannon) // only apply to grabbed components (projectiles)
		{
			// zero angular velocity (degrees)
			HeldComponent->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);

			// Strong angular damping to resist rotation
			HeldComponent->SetAngularDamping(1000.0f);

			// Force rotation to face camera (teleport to avoid physics interpolation fighting us)
			const FRotator DesiredRot = FPSCameraComponent->GetComponentRotation();
			HeldComponent->SetWorldRotation(DesiredRot, false, nullptr, ETeleportType::TeleportPhysics);
		}

		// DEBUG: log grabbed component and target so you can see it's being updated
		if (GEngine)
		{
			UE_LOG(LogTemp, Verbose, TEXT("PhysicsHandle has grabbed component: %s; Target=%s"),
				*GetNameSafe(PhysicsHandle->GrabbedComponent->GetOwner()),
				*HoldLocation.ToString());
		}
	}
}

// Called to bind functionality to input
void AFPSCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))

	{

		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AFPSCharacter::Move);

		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AFPSCharacter::Look);

		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &AFPSCharacter::StartJump);

		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &AFPSCharacter::EndJump);

		EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Started, this, &AFPSCharacter::Fire);

		EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Started, this, &AFPSCharacter::Interact);

	}

	// Direct key binding: Q fires the cannon under crosshair or the held cannon
	PlayerInputComponent->BindKey(EKeys::Q, IE_Pressed, this, &AFPSCharacter::FireCannonUnderCrosshair);

}
void AFPSCharacter::Move(const FInputActionValue& Value)

{

	const FVector2D MovementVector = Value.Get<FVector2D>();

	AddMovementInput(GetActorForwardVector(), MovementVector.X);

	AddMovementInput(GetActorRightVector(), MovementVector.Y);

}



void AFPSCharacter::Look(const FInputActionValue& Value)

{
	
	const FVector2D LookAxisVector = Value.Get<FVector2D>();

	AddControllerYawInput(LookAxisVector.X);

	AddControllerPitchInput(LookAxisVector.Y);

}



void AFPSCharacter::StartJump()

{

	bPressedJump = true;

}



void AFPSCharacter::EndJump()

{

	bPressedJump = false;

}



void AFPSCharacter::Fire()

{

}

void AFPSCharacter::MoveCannonUnderCrosshair()
{
	// Trace to cannon and move it to a point in front of the player
	FVector TraceStart = FPSCameraComponent->GetComponentLocation();
	FVector TraceEnd = TraceStart + FPSCameraComponent->GetForwardVector() * GrabDistance;

	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	if (GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_Visibility, QueryParams))
	{
		if (ACannon* HitCannon = Cast<ACannon>(HitResult.GetActor()))
		{
			// Compute destination in front of player
			FVector Dest = FPSCameraComponent->GetComponentLocation() + FPSCameraComponent->GetForwardVector() * HoldDistance;
			FRotator DestRot = FPSCameraComponent->GetComponentRotation();
			// Move cannon (teleport)
			HitCannon->SetActorLocation(Dest, false, nullptr, ETeleportType::TeleportPhysics);
			HitCannon->SetActorRotation(DestRot, ETeleportType::TeleportPhysics);
			UE_LOG(LogTemp, Log, TEXT("MoveCannonUnderCrosshair: Moved cannon %s to %s"), *GetNameSafe(HitCannon), *Dest.ToString());
		}
	}
}

void AFPSCharacter::FireCannonUnderCrosshair()
{
	// If we're holding a cannon, fire it directly
	if (HeldCannon)
	{
		HeldCannon->Shoot();
		UE_LOG(LogTemp, Log, TEXT("FireCannonUnderCrosshair: Called Shoot() on held cannon %s"), *GetNameSafe(HeldCannon));
		return;
	}

	// Otherwise trace to cannon and call Shoot()
	FVector TraceStart = FPSCameraComponent->GetComponentLocation();
	FVector TraceEnd = TraceStart + FPSCameraComponent->GetForwardVector() * GrabDistance;

	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	if (GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_Visibility, QueryParams))
	{
		if (ACannon* HitCannon = Cast<ACannon>(HitResult.GetActor()))
		{
			HitCannon->Shoot();
			UE_LOG(LogTemp, Log, TEXT("FireCannonUnderCrosshair: Called Shoot() on %s"), *GetNameSafe(HitCannon));
		}
	}
}

void AFPSCharacter::Interact()
{
	// If already holding something, drop it
	if (HeldProjectile || HeldCannon)
	{
		// First release the physics handle so it stops constraining the component
		if (PhysicsHandle && PhysicsHandle->GrabbedComponent)
		{
			PhysicsHandle->ReleaseComponent();
		}

		// Restore physics on the actual component we grabbed
		if (HeldComponent)
		{
			HeldComponent->SetSimulatePhysics(true);
			HeldComponent->SetEnableGravity(true);
			HeldComponent = nullptr;
		}

		// If we were holding a projectile, restore it
		if (HeldProjectile && HeldProjectile->ProjectileMesh)
		{
			HeldProjectile->ProjectileMesh->SetSimulatePhysics(true);
			HeldProjectile->ProjectileMesh->SetEnableGravity(true);
			HeldProjectile->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
			HeldProjectile = nullptr;
		}

		// If we were holding a cannon, restore it
		if (HeldCannon)
		{
			// Detach cannon actor and clear held reference
			HeldCannon->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
			HeldCannon = nullptr;
		}

		return;
	}

	// Line trace from camera to find a projectile or cannon (use GrabDistance here)
	FVector TraceStart = FPSCameraComponent->GetComponentLocation();
	FVector TraceEnd = TraceStart + FPSCameraComponent->GetForwardVector() * GrabDistance;

	// DEBUG: visualize and test trace
	DrawDebugLine(GetWorld(), TraceStart, TraceEnd, FColor::Green, false, 2.0f, 0, 1.0f);
	UE_LOG(LogTemp, Warning, TEXT("Interact: Trace %s -> %s (GrabDistance=%f, HoldDistance=%f)"), *TraceStart.ToString(), *TraceEnd.ToString(), GrabDistance, HoldDistance);

	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	if (GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_Visibility, QueryParams))
	{
		UE_LOG(LogTemp, Log, TEXT("Interact: Hit actor %s (component=%s)"), *GetNameSafe(HitResult.GetActor()), *GetNameSafe(HitResult.GetComponent()));

		// If we hit a cannon, pick it up (same as projectile pickup)
		if (ACannon* HitCannon = Cast<ACannon>(HitResult.GetActor()))
		{
			UPrimitiveComponent* HitComp = HitResult.GetComponent();
			if (HitComp)
			{
				// Ensure physics enabled and disable gravity while held
				HitComp->SetSimulatePhysics(true);
				HitComp->SetEnableGravity(false);

				// store actor and component
				HeldCannon = HitCannon;
				HeldComponent = HitComp;

				if (PhysicsHandle)
				{
					FVector GrabLocation = HitResult.ImpactPoint.IsNearlyZero() ? HitComp->GetComponentLocation() : HitResult.ImpactPoint;
					const FRotator GrabRot = FPSCameraComponent->GetComponentRotation();
					PhysicsHandle->GrabComponentAtLocationWithRotation(HitComp, NAME_None, GrabLocation, GrabRot);

					FVector HoldLocation = FPSCameraComponent->GetComponentLocation() + FPSCameraComponent->GetForwardVector() * HoldDistance;
					PhysicsHandle->SetTargetLocation(HoldLocation);

					HitComp->SetWorldRotation(GrabRot, false, nullptr, ETeleportType::TeleportPhysics);
					UE_LOG(LogTemp, Warning, TEXT("Grabbed cannon %s (comp=%s) — moving to HoldDistance=%f"), *GetNameSafe(HitCannon), *GetNameSafe(HitComp), HoldDistance);
				}
				return;
			}
		}

		// ... existing projectile pickup handling continues ...

		UPrimitiveComponent* HitComp = HitResult.GetComponent();
		AProjectile* Projectile = Cast<AProjectile>(HitResult.GetActor());

		// Log the hit result for debugging
		UE_LOG(LogTemp, Warning, TEXT("Hit: %s; Component: %s"), *GetNameSafe(Projectile), *GetNameSafe(HitComp));

		if (Projectile && HitComp)
		{
			// Use the actual component we hit. Ensure it's simulating physics and disable gravity.
			HitComp->SetSimulatePhysics(true);
			HitComp->SetEnableGravity(false);

			// Immediately zero angular velocity and apply heavy angular damping to prevent spinning
			HitComp->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
			HitComp->SetAngularDamping(1000.0f);

			// store both actor and component
			HeldProjectile = Projectile;
			HeldComponent = HitComp;

			if (PhysicsHandle)
			{
				// Grab the exact component at the impact location (or component location)
				FVector GrabLocation = HitResult.ImpactPoint.IsNearlyZero() ? HitComp->GetComponentLocation() : HitResult.ImpactPoint;

				// If available, prefer grabbing with an explicit rotation to lock orientation.
				const FRotator GrabRot = FPSCameraComponent->GetComponentRotation();
				PhysicsHandle->GrabComponentAtLocationWithRotation(HitComp, NAME_None, GrabLocation, GrabRot);

				// Immediately move it to the hold location (shorter HoldDistance)
				FVector HoldLocation = FPSCameraComponent->GetComponentLocation() + FPSCameraComponent->GetForwardVector() * HoldDistance;
				PhysicsHandle->SetTargetLocation(HoldLocation);

				// Also ensure the component rotation is the desired one right away
				HitComp->SetWorldRotation(GrabRot, false, nullptr, ETeleportType::TeleportPhysics);

				UE_LOG(LogTemp, Warning, TEXT("Grabbed component %s of %s — moving to HoldDistance=%f"), *GetNameSafe(HitComp), *GetNameSafe(Projectile), HoldDistance);
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Interact: No valid projectile/component hit"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Verbose, TEXT("Interact: Trace missed"));
	}
}

