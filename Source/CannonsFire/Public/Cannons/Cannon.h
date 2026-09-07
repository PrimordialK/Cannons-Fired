// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Cannons/Projectile.h"
#include "Cannon.generated.h"

class USceneComponent;
class UArrowComponent;
class AProjectile;

UCLASS()
class CANNONSFIRE_API ACannon : public AActor
{
	GENERATED_BODY()
	
public:	
	ACannon();

	virtual void OnConstruction(const FTransform& Transform) override;

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

	// Root so components can be positioned in the editor
	UPROPERTY(VisibleAnywhere, Category = "Components")
	USceneComponent* SceneRoot;

	// Spawn point for projectiles (move/rotate in viewport)
	UPROPERTY(VisibleAnywhere, Category = "Shooting")
	UArrowComponent* MuzzlePoint;

	// Projectile to spawn
	UPROPERTY(EditAnywhere, Category = "Shooting")
	TSubclassOf<AProjectile> ProjectileClass;

	// Initial launch speed for spawned projectile
	UPROPERTY(EditAnywhere, Category = "Shooting", meta=(ClampMin="100.0", UIMin="100.0", UIMax="10000.0"))
	float LaunchSpeed = 1500.0f;

	// Spawn & launch a projectile from the muzzle
	UFUNCTION(BlueprintCallable, Category = "Shooting")
	void Shoot();
};

