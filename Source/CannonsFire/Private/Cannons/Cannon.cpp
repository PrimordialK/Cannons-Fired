// Fill out your copyright notice in the Description page of Project Settings.

#include "Cannons/Cannon.h"
#include "Cannons/Projectile.h"
#include "Components/SceneComponent.h"
#include "Components/ArrowComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
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

	// Robust search for the cannon head mesh component and attach the muzzle to it.
	// Matches common names: "CannonHead", "CannonHeadMesh", "Cannon_Head", "StaticMesh1" (fallback).
	UStaticMeshComponent* FoundHead = nullptr;
	TArray<UStaticMeshComponent*> MeshComps;
	GetComponents<UStaticMeshComponent>(MeshComps);

	for (UStaticMeshComponent* Comp : MeshComps)
	{
		if (!Comp) continue;

		const FString CompName = Comp->GetName();

		// Check for common candidate names or a "CannonHead" substring
		if (CompName.Equals(TEXT("CannonHead"), ESearchCase::IgnoreCase) ||
			CompName.Equals(TEXT("CannonHeadMesh"), ESearchCase::IgnoreCase) ||
			CompName.Equals(TEXT("Cannon_Head"), ESearchCase::IgnoreCase) ||
			CompName.Equals(TEXT("StaticMesh1"), ESearchCase::IgnoreCase) ||
			CompName.Contains(TEXT("CannonHead")) ||
			CompName.Contains(TEXT("Cannon_Head")) )
		{
			FoundHead = Comp;
			break;
		}
	}

	if (FoundHead && MuzzlePoint)
	{
		MuzzlePoint->AttachToComponent(FoundHead, FAttachmentTransformRules::KeepRelativeTransform);
		UE_LOG(LogTemp, Log, TEXT("ACannon::OnConstruction - Attached MuzzlePoint to cannon head component %s"), *GetNameSafe(FoundHead));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("ACannon::OnConstruction - Cannon head mesh not found. MuzzlePoint remains attached to SceneRoot. (Check component name in BP)"));
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

	const FVector Forward = MuzzlePoint->GetForwardVector();
	const FVector SpawnLoc = MuzzlePoint->GetComponentLocation() + Forward * 20.0f; // tweak offset as needed
	const FRotator SpawnRot = MuzzlePoint->GetComponentRotation();

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = GetInstigator();
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// Debug: visualize spawn
	DrawDebugLine(World, MuzzlePoint->GetComponentLocation(), SpawnLoc + Forward * 50.0f, FColor::Red, false, 2.0f, 0, 2.0f);
	UE_LOG(LogTemp, Log, TEXT("ACannon::Shoot - Spawning projectile at %s rot %s"), *SpawnLoc.ToString(), *SpawnRot.ToString());

	// Play one-shot fire sound at the muzzle location
	if (FireSound)
	{
		const FVector SoundLocation = MuzzlePoint->GetComponentLocation();
		UGameplayStatics::PlaySoundAtLocation(this, FireSound, SoundLocation);
	}

	// Spawn
	AProjectile* NewProj = World->SpawnActor<AProjectile>(ProjectileClass, SpawnLoc, SpawnRot, SpawnParams);
	if (!NewProj)
	{
		UE_LOG(LogTemp, Warning, TEXT("ACannon::Shoot - failed to spawn projectile"));
		return;
	}

	// Ensure actor-level collision is enabled
	NewProj->SetActorEnableCollision(true);

	// Force-enable collision on all primitive components
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
		Prim->RecreatePhysicsState();
		if (Prim->IsSimulatingPhysics()) Prim->WakeRigidBody();
		UE_LOG(LogTemp, Log, TEXT("ACannon::Shoot - Enabled collision on component %s"), *GetNameSafe(Prim));
	}

	// Launch logic (prefer ProjectileMovementComponent, fallback to physics)... 
	if (UProjectileMovementComponent* ProjMove = NewProj->FindComponentByClass<UProjectileMovementComponent>())
	{
		ProjMove->SetUpdatedComponent(NewProj->ProjectileMesh ? NewProj->ProjectileMesh : Cast<UPrimitiveComponent>(NewProj->GetRootComponent()));
		ProjMove->Velocity = Forward * LaunchSpeed;
		ProjMove->Activate(true);
	}
	else
	{
		UPrimitiveComponent* VelocityTarget = NewProj->ProjectileMesh ? NewProj->ProjectileMesh : Cast<UPrimitiveComponent>(PrimComps.Num() ? PrimComps[0] : nullptr);
		if (VelocityTarget)
		{
			VelocityTarget->SetSimulatePhysics(true);
			VelocityTarget->SetEnableGravity(true);
			VelocityTarget->RecreatePhysicsState();
			VelocityTarget->SetPhysicsLinearVelocity(Forward * LaunchSpeed, false);
			VelocityTarget->WakeRigidBody();
		}
	}

	UE_LOG(LogTemp, Log, TEXT("ACannon: Spawned projectile %s at %s"), *GetNameSafe(NewProj), *SpawnLoc.ToString());
}