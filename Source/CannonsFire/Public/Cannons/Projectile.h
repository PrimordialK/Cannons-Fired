// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Projectile.generated.h"

class UMaterialInterface;

UCLASS()
class CANNONSFIRE_API AProjectile : public AActor
{
	GENERATED_BODY()
	
public:	
	AProjectile();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(VisibleAnywhere, Category = "Projectile")
	UStaticMeshComponent* ProjectileMesh;

	UPROPERTY(VisibleAnywhere, Category = "Projectile")
	USphereComponent* PickupSphere;

	// If true the projectile will use physics (simulate) rather than rely on ProjectileMovementComponent.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	bool bForcePhysicsMovement = false;

	// Material to apply to the mesh at BeginPlay
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
	UMaterialInterface* ProjectileMaterial = nullptr;

	// Delay (seconds) after spawn before enabling collision on the projectile.
	// Small values like 0.02 - 0.1 work well to avoid initial overlap with the cannon.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collision", meta=(ClampMin="0.0", UIMin="0.0", UIMax="1.0"))
	float CollisionEnableDelay = 0.05f;

private:
	FTimerHandle CollisionEnableTimerHandle;

	UFUNCTION()
	void EnableCollisionDeferred();
};
