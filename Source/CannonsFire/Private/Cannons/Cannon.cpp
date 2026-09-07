// Fill out your copyright notice in the Description page of Project Settings.

#include "Cannons/Cannon.h"
#include "Cannons/Projectile.h"
#include "Components/SceneComponent.h"
#include "Components/ArrowComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "DrawDebugHelpers.h"
#include "Components/PrimitiveComponent.h"

// Sets default values
ACannon::ACannon()
{
	PrimaryActorTick.bCanEverTick = true;

	// scene root to allow independent placement in editor
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;
	SceneRoot->SetMobility(EComponentMobility::Movable);

	// muzzle / spawn point
	MuzzlePoint = CreateDefaultSubobject<UArrowComponent>(TEXT("MuzzlePoint"));
	MuzzlePoint->SetupAttachment(SceneRoot);
	MuzzlePoint->SetRelativeLocation(FVector(100.f, 0.f, 0.f));
	MuzzlePoint->SetMobility(EComponentMobility::Movable);
}

void ACannon::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (SceneRoot)
	{
		SceneRoot->SetMobility(EComponentMobility::Movable);
	}
	if (MuzzlePoint)
	{
		MuzzlePoint->SetMobility(EComponentMobility::Movable);
	}
}

void ACannon::BeginPlay()
{
	Super::BeginPlay();
}

void ACannon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ACannon::Shoot()
{
	// Basic validation
	if (!ProjectileClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("ACannon::Shoot - ProjectileClass not set on %s"), *GetName());
		return;
	}
	if (!MuzzlePoint)
	{
		UE_LOG(LogTemp, Warning, TEXT("ACannon::Shoot - MuzzlePoint missing on %s"), *GetName());
		return;
	}

	UWorld* World = GetWorld();
	if (!World) return;

	// Spawn a bit further in front of the muzzle to avoid initial overlap
	const FVector Forward = MuzzlePoint->GetForwardVector();
	const float SpawnForwardOffset = 40.0f; // increased from 20.0f
	const FVector SpawnLoc = MuzzlePoint->GetComponentLocation() + Forward * SpawnForwardOffset;
	const FRotator SpawnRot = MuzzlePoint->GetComponentRotation();

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = GetInstigator();
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	DrawDebugLine(World, MuzzlePoint->GetComponentLocation(), SpawnLoc + Forward * 50.0f, FColor::Red, false, 2.0f, 0, 2.0f);
	UE_LOG(LogTemp, Log, TEXT("ACannon::Shoot - Spawning projectile at %s rot %s"), *SpawnLoc.ToString(), *SpawnRot.ToString());

	AProjectile* NewProj = World->SpawnActor<AProjectile>(ProjectileClass, SpawnLoc, SpawnRot, SpawnParams);
	if (!NewProj)
	{
		UE_LOG(LogTemp, Warning, TEXT("ACannon::Shoot - failed to spawn projectile"));
		return;
	}

	// Make the projectile ignore the cannon actor while it leaves the muzzle to avoid immediate collisions
	// This calls the primitive helper that tells the movement system to ignore collisions with the cannon actor
	TArray<UPrimitiveComponent*> PrimCompsIgnore;
	NewProj->GetComponents<UPrimitiveComponent>(PrimCompsIgnore);
	for (UPrimitiveComponent* Prim : PrimCompsIgnore)
	{
		if (!Prim) continue;
		Prim->IgnoreActorWhenMoving(this, true);
	}

	// Additionally, ignore the cannon's individual primitive components to be safe (per-component ignore)
	TArray<UPrimitiveComponent*> CannonPrims;
	GetComponents<UPrimitiveComponent>(CannonPrims); // cannon's components
	if (CannonPrims.Num() > 0)
	{
		for (UPrimitiveComponent* Prim : PrimCompsIgnore)
		{
			if (!Prim) continue;
			for (UPrimitiveComponent* CPrim : CannonPrims)
			{
				if (!CPrim) continue;
				// Ignore specific cannon component when moving to avoid any component-vs-component collision checks
				Prim->IgnoreComponentWhenMoving(CPrim, true);
			}
		}
	}

	// Ensure actor-level collision enabled
	NewProj->SetActorEnableCollision(true);

	// Collect primitive components and force collision/simulation, then reinitialize physics state
	TArray<UPrimitiveComponent*> PrimComps;
	NewProj->GetComponents<UPrimitiveComponent>(PrimComps);

	for (UPrimitiveComponent* Prim : PrimComps)
	{
		if (!Prim) continue;

		Prim->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Prim->SetCollisionObjectType(ECC_PhysicsBody);
		Prim->SetCollisionResponseToAllChannels(ECR_Block);
		Prim->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
		Prim->SetNotifyRigidBodyCollision(true);

		// Recreate physics state so runtime changes take effect, then wake the body
		Prim->RecreatePhysicsState();
		if (Prim->IsSimulatingPhysics())
		{
			Prim->WakeRigidBody();
		}

		UE_LOG(LogTemp, Log, TEXT("ACannon::Shoot - Enabled collision on component %s (Enabled=%d, ObjType=%d)"),
			*GetNameSafe(Prim),
			(int)Prim->GetCollisionEnabled(),
			(int)Prim->GetCollisionObjectType());
	}

	// If projectile requests physics movement force fallback
	UProjectileMovementComponent* ProjMove = NewProj->FindComponentByClass<UProjectileMovementComponent>();
	if (NewProj->bForcePhysicsMovement)
	{
		UE_LOG(LogTemp, Log, TEXT("ACannon::Shoot - Projectile requests physics movement, using physics fallback"));
		ProjMove = nullptr;
	}

	UPrimitiveComponent* MovementUpdatedComp = nullptr;
	if (NewProj->ProjectileMesh)
	{
		MovementUpdatedComp = NewProj->ProjectileMesh;
	}
	if (!MovementUpdatedComp && PrimComps.Num() > 0)
	{
		MovementUpdatedComp = PrimComps[0];
	}

	if (ProjMove)
	{
		if (MovementUpdatedComp)
		{
			ProjMove->SetUpdatedComponent(MovementUpdatedComp);
		}
		ProjMove->Velocity = Forward * LaunchSpeed;
		ProjMove->Activate(true);
		UE_LOG(LogTemp, Log, TEXT("ACannon::Shoot - set ProjectileMovement velocity %s"), *ProjMove->Velocity.ToString());
	}
	else
	{
		// Robust physics fallback: ensure physics is active, recreate state, enable CCD, then apply impulse
		UPrimitiveComponent* VelocityTarget = MovementUpdatedComp;
		if (VelocityTarget)
		{
			// Make sure it's simulating physics
			if (!VelocityTarget->IsSimulatingPhysics())
			{
				VelocityTarget->SetSimulatePhysics(true);
			}

			// Ensure CCD is on for fast bodies
			if (FBodyInstance* BI = VelocityTarget->GetBodyInstance())
			{
				BI->bUseCCD = true;
			}

			// Recreate/wake so the body is ready
			VelocityTarget->RecreatePhysicsState();
			VelocityTarget->WakeRigidBody();

			// Log mass and sim state for debugging
			float Mass = VelocityTarget->GetMass();
			UE_LOG(LogTemp, Log, TEXT("ACannon::Shoot - Physics fallback target=%s sim=%d mass=%f"),
				*GetNameSafe(VelocityTarget), (int)VelocityTarget->IsSimulatingPhysics(), Mass);

			// Use an impulse (mass-scaled) so the body responds immediately even if it was asleep or just created
			const FVector Impulse = Forward * LaunchSpeed * Mass;
			VelocityTarget->AddImpulse(Impulse, NAME_None, true);
			VelocityTarget->WakeRigidBody();

			// Small safety: also set linear velocity to desired value (non-authoritative, but useful)
			VelocityTarget->SetPhysicsLinearVelocity(Forward * LaunchSpeed, false);

			UE_LOG(LogTemp, Log, TEXT("ACannon::Shoot - applied physics impulse %s (impulse vector %s)"), *GetNameSafe(VelocityTarget), *Impulse.ToString());
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("ACannon::Shoot - no primitive component found to apply physics velocity"));
		}
	}

	UE_LOG(LogTemp, Log, TEXT("ACannon: Spawned projectile %s at %s"), *GetNameSafe(NewProj), *SpawnLoc.ToString());
}