// Fill out your copyright notice in the Description page of Project Settings.


#include "HUDS/MainMenuHUD.h"
#include "Widgets/SWeakWidget.h"


void AMainMenuHUD::BeginPlay()
{
	Super::BeginPlay();

	// Spawn and Show Widget
	SpawnMainMenuWidget();
}

void AMainMenuHUD::DrawHUD()
{
	Super::DrawHUD();

}





//void AMainMenuHUD::ShowMainMenuWidget()
//{
//	/*MainMenuCompoundWidget = SNew(SMainMenuCompoundWidget);
//	GEngine->GameViewport->AddViewportWidgetContent(SAssignNew(MainMenuCompoundWidgetContainer, SWeakWidget).PossiblyNullContent(MainMenuCompoundWidget.ToSharedRef()));*/
//
//
//
//	PlayerOwner->bShowMouseCursor = true;
//	PlayerOwner->SetInputMode(FInputModeUIOnly());
//}

void AMainMenuHUD::SpawnMainMenuWidget()
{

	if (!MainMenuWidgetClass) return;
	// Delete game menu widget if it already exists

	if (MainMenuWidgetContainer) {
		MainMenuWidgetContainer->RemoveFromParent();
		MainMenuWidgetContainer = nullptr;

	}

	MainMenuWidgetContainer = CreateWidget<UMainMenuWidget>(GetWorld(), MainMenuWidgetClass);
	MainMenuWidgetContainer->AddToViewport();



	PlayerOwner->bShowMouseCursor = true;
	PlayerOwner->SetInputMode(FInputModeGameOnly());
}
