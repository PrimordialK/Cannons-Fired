// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/Character.h"
#include "PhysicsEngine/PhysicsHandleComponent.h"
#include "FPSCharacter.generated.h"

class UInputMappingContext;
class UInputAction;
class AProjectile;
class USpringArmComponent;
class UPrimitiveComponent; // forward declare

UCLASS()
class CANNONSFIRE_API AFPSCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AFPSCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputMappingContext* PlayerMappingContext;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputAction* MoveAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputAction* LookAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputAction* JumpAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputAction* FireAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputAction* InteractAction;

	UPROPERTY(VisibleAnywhere, Category = "Mesh")
	USkeletalMeshComponent* HeroMeshComponent;

	// Camera boom to handle rotation cleanly
	UPROPERTY(VisibleAnywhere, Category = "Camera")
	USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere)
	UCameraComponent* FPSCameraComponent;

	UPROPERTY(VisibleAnywhere, Category = "Pickup")
	UPhysicsHandleComponent* PhysicsHandle;

	// How far in front of camera to hold the object (shorter — closer to player)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup", meta=(ClampMin="20.0", UIMin="20.0", UIMax="1000.0"))
	float HoldDistance = 100.0f;

	// How far away the player can grab an object (trace distance)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup", meta=(ClampMin="50.0", UIMin="50.0", UIMax="3000.0"))
	float GrabDistance = 600.0f;

	UFUNCTION()
	void Move(const FInputActionValue& Value);

	UFUNCTION()
	void Look(const FInputActionValue& Value);

	UFUNCTION()
	void StartJump();

	UFUNCTION()
	void EndJump();

	UFUNCTION()
	void Fire();

	UFUNCTION()
	void Interact();

private:
	AProjectile* HeldProjectile;

	// Track the actual component we grabbed so drop restores physics correctly
	UPrimitiveComponent* HeldComponent = nullptr;
};
