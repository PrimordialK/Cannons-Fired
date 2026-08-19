// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/Canvas.h"
#include "GameFramework/HUD.h"
#include "GUI/GameWidget.h"
#include "GamesHUD.generated.h"


/**
 * 
 */
UCLASS()
class CANNONSFIRE_API AGamesHUD : public AHUD
{
	GENERATED_BODY()

public:

	virtual void BeginPlay() override;

	virtual void DrawHUD() override;
	
	UPROPERTY(EditAnywhere)
	UTexture2D* CrosshairTexture;
	
	UPROPERTY(EditAnywhere)
	TSubclassOf<class UGameWidget> GameWidgetClass;
	UGameWidget* GameWidgetContainer;

	void SpawnGameWidget();
};
