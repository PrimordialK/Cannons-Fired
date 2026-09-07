// Fill out your copyright notice in the Description page of Project Settings.

#include "Player/FPSCharacter.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "Cannons/Projectile.h"
#include "Components/SphereComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerInput.h"
#include "GameFramework/InputSettings.h"
#include "Engine/World.h"
#include "Engine/Engine.h" // for GEngine / AddOnScreenDebugMessage

// Sets default values
AFPSCharacter::AFPSCharacter()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	HeroMeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("HeroMeshComponent")); // Add Component in Unity

	//FPSMeshComponent->SetupAttachment(Camera); 

	HeroMeshComponent->bCastDynamicShadow = false;

	HeroMeshComponent->CastShadow = false;

	// Create camera boom (spring arm) and attach to capsule; boom will apply controller rotation
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetCapsuleComponent());
	CameraBoom->TargetArmLength = 0.0f; // first-person: keep the camera at the capsule location
	CameraBoom->bUsePawnControlRotation = true; // boom rotates with controller

	// Camera attached to boom
	FPSCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FPSCamera"));
	FPSCameraComponent->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FPSCameraComponent->bUsePawnControlRotation = false; // camera itself should not additionally use controller rotation

	PhysicsHandle = CreateDefaultSubobject<UPhysicsHandleComponent>(TEXT("PhysicsHandle"));

	HeldProjectile = nullptr;

	// keep default character rotation behavior; boom handles visual rotation for look
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;
}

// Called when the game starts or when spawned
void AFPSCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Diagnostic: confirm BeginPlay runs and controller possession
	UE_LOG(LogTemp, Warning, TEXT("AFPSCharacter::BeginPlay - this=%p"), this);
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, TEXT("AFPSCharacter::BeginPlay"));

	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (!PlayerController)
	{
		UE_LOG(LogTemp, Error, TEXT("AFPSCharacter::BeginPlay - no PlayerController (pawn not possessed?)"));
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("No PlayerController - pawn not possessed"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("AFPSCharacter::BeginPlay - PlayerController=%p Local=%d"), PlayerController, PlayerController->IsLocalController());
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, FString::Printf(TEXT("PC present, IsLocal=%d"), PlayerController->IsLocalController()));
	}

	if (PlayerController)
	{
		// Add enhanced input mapping context
		if (PlayerMappingContext)
		{
			UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer());
			if (Subsystem)
			{
				Subsystem->AddMappingContext(PlayerMappingContext, 0);

				// Debug: confirm mapping context was added
				UE_LOG(LogTemp, Warning, TEXT("AFPSCharacter::BeginPlay - Added PlayerMappingContext"));
				if (GEngine)
				{
					GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Yellow, TEXT("Enhanced Input mapping context added"));
				}
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("AFPSCharacter::BeginPlay - Failed to get EnhancedInput subsystem"));
				if (GEngine)
				{
					GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("Failed to get EnhancedInput subsystem"));
				}
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("AFPSCharacter::BeginPlay - PlayerMappingContext is null"));
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("PlayerMappingContext is null"));
			}
		}

		// Ensure mouse input controls the camera in-game
		PlayerController->bShowMouseCursor = false;
		FInputModeGameOnly InputMode;
		PlayerController->SetInputMode(InputMode);
	}
	
}

// Called every frame
void AFPSCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Update held object position every frame
	if (PhysicsHandle && PhysicsHandle->GrabbedComponent)
	{
		FVector HoldLocation = FPSCameraComponent->GetComponentLocation()
			+ FPSCameraComponent->GetForwardVector() * HoldDistance;
		PhysicsHandle->SetTargetLocation(HoldLocation);
	}

	// Temporary fallback test inside Tick()
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (PC)
	{
		float DX = 0.f, DY = 0.f;
		PC->GetInputMouseDelta(DX, DY);

		// Always print raw mouse delta (even if zero) to confirm capture
		UE_LOG(LogTemp, Verbose, TEXT("Tick raw mouse delta DX=%f DY=%f"), DX, DY);

		if (FMath::Abs(DX) > KINDA_SMALL_NUMBER || FMath::Abs(DY) > KINDA_SMALL_NUMBER)
		{
			const float MouseSensitivity = 1.0f;
			AddControllerYawInput(DX * MouseSensitivity);
			AddControllerPitchInput(-DY * MouseSensitivity); // invert Y if needed

			// Debug: report raw mouse delta (visible in Output / on-screen)
			UE_LOG(LogTemp, Warning, TEXT("Tick fallback mouse delta DX=%f DY=%f"), DX, DY);
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(1, 0.1f, FColor::Cyan, FString::Printf(TEXT("MouseDX=%0.2f DY=%0.2f"), DX, DY));
			}
		}
	}
}

// Called to bind functionality to input
void AFPSCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UE_LOG(LogTemp, Warning, TEXT("AFPSCharacter::SetupPlayerInputComponent called PlayerInputComponent=%p"), PlayerInputComponent);
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Blue, TEXT("SetupPlayerInputComponent"));

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		UE_LOG(LogTemp, Warning, TEXT("EnhancedInputComponent present=%p"), EnhancedInputComponent);

		// Log whether actions exist
		UE_LOG(LogTemp, Warning, TEXT("MoveAction=%p LookAction=%p"), MoveAction, LookAction);
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(3, 3.0f, FColor::Blue, FString::Printf(TEXT("Move=%p Look=%p"), MoveAction, LookAction));
		}

		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AFPSCharacter::Move);
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AFPSCharacter::Look);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &AFPSCharacter::StartJump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &AFPSCharacter::EndJump);
		EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Started, this, &AFPSCharacter::Fire);
		EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Started, this, &AFPSCharacter::Interact);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("AFPSCharacter::SetupPlayerInputComponent - EnhancedInputComponent not present. Is the PlayerInput subsystem properly initialized?"));
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("No EnhancedInputComponent"));
	}
}