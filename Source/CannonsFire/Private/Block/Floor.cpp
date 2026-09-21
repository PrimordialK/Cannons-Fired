// Fill out your copyright notice in the Description page of Project Settings.


#include "Block/Floor.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"
#include "HUDS/GamesHUD.h"
#include "Block/Blocks.h"

// Sets default values
AFloor::AFloor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Create box collision as the root and enable hit notifications
	CollisionComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionComponent"));
	RootComponent = CollisionComponent;
	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CollisionComponent->SetCollisionResponseToAllChannels(ECR_Block);
	CollisionComponent->SetNotifyRigidBodyCollision(true);
	CollisionComponent->BodyInstance.SetCollisionProfileName("BlockAll");
	CollisionComponent->SetGenerateOverlapEvents(false);

	// Bind hit event
	CollisionComponent->OnComponentHit.AddDynamic(this, &AFloor::OnHit);
}

// Called when the game starts or when spawned
void AFloor::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AFloor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AFloor::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (!OtherActor || OtherActor == this) return;

	// Only count hits from block actors
	if (ABlocks* HitBlock = Cast<ABlocks>(OtherActor))
	{
		// Get player HUD and add score
		if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
		{
			if (AGamesHUD* HUD = Cast<AGamesHUD>(PC->GetHUD()))
			{
				HUD->AddScore(1);
			}
		}

		// Optionally destroy or mark the block (prevents double-counting)
		HitBlock->Destroy();
	}
}

