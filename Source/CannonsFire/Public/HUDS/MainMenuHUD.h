// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/Canvas.h"
#include "GameFramework/HUD.h"
#include "GUI/MainMenuWidget.h"
#include "MainMenuHUD.generated.h"

/**
 * 
 */
UCLASS()
class CANNONSFIRE_API AMainMenuHUD : public AHUD
{
	GENERATED_BODY()
public:

	virtual void BeginPlay() override;

	virtual void DrawHUD() override;

	/*void ShowMainMenuWidget();*/
	UPROPERTY(EditAnywhere)
	TSubclassOf<class UMainMenuWidget> MainMenuWidgetClass;

	UMainMenuWidget* MainMenuWidgetContainer;



	void SpawnMainMenuWidget();
};
