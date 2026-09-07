// Fill out your copyright notice in the Description page of Project Settings.

#include "Cannons/Projectile.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "PhysicsEngine/BodyInstance.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/Engine.h"
#include "Materials/MaterialInterface.h"
#include "TimerManager.h"

// Sets default values
AProjectile::AProjectile()
{
	PrimaryActorTick.bCanEverTick = true;

	ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ProjectileMesh"));
	ProjectileMesh->SetSimulatePhysics(true);
	ProjectileMesh->SetCollisionProfileName(TEXT("PhysicsActor"));

	// Make sure it registers as a physics body and blocks visibility traces
	ProjectileMesh->SetCollisionObjectType(ECC_PhysicsBody);
	ProjectileMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	RootComponent = ProjectileMesh;

	PickupSphere = CreateDefaultSubobject<USphereComponent>(TEXT("PickupSphere"));
	PickupSphere->SetSphereRadius(150.0f);
	PickupSphere->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	PickupSphere->SetupAttachment(RootComponent);

	// sensible default
	CollisionEnableDelay = 0.05f;
}

void AProjectile::BeginPlay()
{
	Super::BeginPlay();

	// Apply material if one is assigned (Blueprint or details)
	if (ProjectileMaterial && ProjectileMesh)
	{
		ProjectileMesh->SetMaterial(0, ProjectileMaterial);
	}

	// If a small delay is requested, temporarily disable collision then re-enable after the timer.
	if (CollisionEnableDelay > KINDA_SMALL_NUMBER)
	{
		// Disable collision immediately so spawn overlap doesn't resolve against world/cannon
		TArray<UPrimitiveComponent*> PrimComps;
		GetComponents<UPrimitiveComponent>(PrimComps);
		for (UPrimitiveComponent* Prim : PrimComps)
		{
			if (!Prim) continue;
			Prim->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}

		// Start a single-shot timer that will enable collision and reinit physics
		if (GetWorld())
		{
			GetWorldTimerManager().SetTimer(CollisionEnableTimerHandle, this, &AProjectile::EnableCollisionDeferred, CollisionEnableDelay, false);
		}
	}
	else
	{
		// Immediately enable/initialize collision if delay is 0
		EnableCollisionDeferred();
	}

	// Movement component handling and other initializations remain; we still set up ProjectileMovement below
	// (EnableCollisionDeferred will also perform RecreatePhysicsState and CCD setup)
}

void AProjectile::EnableCollisionDeferred()
{
	// Ensure actor collision is enabled
	SetActorEnableCollision(true);

	// Force collision + physics on primary mesh and any primitive components so runtime spawn has valid collision
	TArray<UPrimitiveComponent*> PrimComps;
	GetComponents<UPrimitiveComponent>(PrimComps);

	for (UPrimitiveComponent* Prim : PrimComps)
	{
		if (!Prim) continue;

		// Enable collision for queries and physics
		Prim->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

		// Prefer PhysicsActor behavior for physics-driven projectiles
		Prim->SetCollisionProfileName(TEXT("PhysicsActor"));
		Prim->SetCollisionObjectType(ECC_PhysicsBody);
		Prim->SetCollisionResponseToAllChannels(ECR_Block);
		Prim->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore); // optional: don't collide with player immediately
		Prim->SetNotifyRigidBodyCollision(true);

		// Enable CCD on body instance to avoid tunneling
		if (FBodyInstance* BI = Prim->GetBodyInstance())
		{
			BI->bUseCCD = true;
		}

		// If it's the visual mesh prefer simulating physics (keeps consistent behavior)
		if (Prim == ProjectileMesh)
		{
			Prim->SetSimulatePhysics(true);
		}

		// Recreate physics state and wake so changes take effect immediately
		Prim->RecreatePhysicsState();
		if (Prim->IsSimulatingPhysics())
		{
			Prim->WakeRigidBody();
		}

		UE_LOG(LogTemp, Verbose, TEXT("AProjectile::EnableCollisionDeferred - prim %s collision=%d sim=%d"),
			*GetNameSafe(Prim),
			(int)Prim->GetCollisionEnabled(),
			(int)Prim->IsSimulatingPhysics());
	}

	// If the projectile has a ProjectileMovementComponent, ensure it updates the visible/physics primitive
	if (UProjectileMovementComponent* ProjMove = FindComponentByClass<UProjectileMovementComponent>())
	{
		// If the blueprint/instance requests physics, disable movement component and prefer physics
		if (bForcePhysicsMovement)
		{
			ProjMove->Deactivate();
			UE_LOG(LogTemp, Verbose, TEXT("AProjectile::EnableCollisionDeferred - bForcePhysicsMovement true, deactivated ProjectileMovement"));
		}
		else
		{
			if (ProjectileMesh && ProjectileMesh->GetCollisionEnabled() != ECollisionEnabled::NoCollision)
			{
				ProjMove->SetUpdatedComponent(ProjectileMesh);
			}
			else if (PrimComps.Num() > 0)
			{
				ProjMove->SetUpdatedComponent(PrimComps[0]);
			}

			// Improve accuracy for fast projectiles
			ProjMove->bForceSubStepping = true;
			ProjMove->MaxSimulationTimeStep = 0.01f;
			ProjMove->MaxSimulationIterations = 8;
			ProjMove->Activate(true);

			UE_LOG(LogTemp, Verbose, TEXT("AProjectile::EnableCollisionDeferred - ProjectileMovement UpdatedComponent=%s"),
				*GetNameSafe(ProjMove->UpdatedComponent));
		}
	}
}

void AProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

