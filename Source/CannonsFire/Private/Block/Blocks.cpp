// Fill out your copyright notice in the Description page of Project Settings.


#include "Block/Blocks.h"

// Sets default values
ABlocks::ABlocks()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void ABlocks::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ABlocks::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

