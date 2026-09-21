// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Floor.generated.h"

class UBoxComponent;

UCLASS()
class CANNONSFIRE_API AFloor : public AActor
{
	GENERATED_BODY()
	
public:	
	AFloor();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

	// Collision component to receive hits
	UPROPERTY(VisibleAnywhere, Category="Collision")
	UBoxComponent* CollisionComponent;

	// Hit handler
	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
};
