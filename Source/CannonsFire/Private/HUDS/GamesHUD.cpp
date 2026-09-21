// Fill out your copyright notice in the Description page of Project Settings.



#include "HUDS/GamesHUD.h"
#include "GUI/WinWidget.h"


void AGamesHUD::BeginPlay()
{
	Super::BeginPlay();
	Score = 0;
	SpawnGameWidget();
}

void AGamesHUD::DrawHUD()
{
	Super::DrawHUD();

	if(!CrosshairTexture) return;
	

	//Canvas
	float CanvasWidth = Canvas->ClipX;
	float CanvasHeight = Canvas->ClipY;
	FVector2D Center(CanvasWidth * 0.5f, CanvasHeight * 0.5f);

	// Crosshair
	float CrosshairWidth = CrosshairTexture->GetSurfaceWidth();
	float CrosshairHeight = CrosshairTexture->GetSurfaceHeight();


	float AlignmentX = 0.5f;
	float AlignmentY = 0.5f;
	FVector2D CrosshairPosOffset(CrosshairWidth * AlignmentX, CrosshairHeight * AlignmentY);



	// Draw Settings
	FVector2D CrosshairDrawPos = Center - CrosshairPosOffset;


	float CrosshairScale = 1.0f;
	FVector2D CrosshairDrawSize(CrosshairWidth * CrosshairScale, CrosshairHeight * CrosshairScale);



	// Draw the crosshair

	FCanvasTileItem CrosshairTileItem(CrosshairDrawPos, CrosshairTexture->GetResource(), CrosshairDrawSize, FLinearColor::White);
	CrosshairTileItem.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(CrosshairTileItem);

}

void AGamesHUD::SpawnGameWidget()
{
	if (!GameWidgetClass) return;

	// Delete game menu widget if it already exists

	if (GameWidgetContainer) {
		GameWidgetContainer->RemoveFromParent();
		GameWidgetContainer = nullptr;

	}

	GameWidgetContainer = CreateWidget<UGameWidget>(GetWorld(), GameWidgetClass);
	if (GameWidgetContainer)
	{
		GameWidgetContainer->AddToViewport();
		GameWidgetContainer->UpdateScore(Score);
		
		
	}

	if (PlayerOwner)
	{
		PlayerOwner->bShowMouseCursor = false;
		PlayerOwner->SetInputMode(FInputModeGameOnly());
	}
}

void AGamesHUD::AddScore(int32 Delta)
{
	Score += Delta;
	if (GameWidgetContainer)
	{
		GameWidgetContainer->UpdateScore(Score);
	}
}

void AGamesHUD::OnTimerEnded()
{
	// Handle timer end (minimal implementation to satisfy the delegate bind).
	// Customize: show end-screen widget, stop game input, etc.

	// Remove the game widget if present
	if (GameWidgetContainer)
	{
		GameWidgetContainer->RemoveFromParent();
		GameWidgetContainer = nullptr;
	}

	// Optionally show mouse cursor / switch input to UI
	if (PlayerOwner)
	{
		PlayerOwner->bShowMouseCursor = true;
		PlayerOwner->SetInputMode(FInputModeUIOnly());
	}
}

