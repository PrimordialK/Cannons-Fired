// Fill out your copyright notice in the Description page of Project Settings.


#include "GUI/GameWidget.h"

void UGameWidget::NativeConstruct()
{
	Super::NativeConstruct();
	UpdateTimer(60);
	UpdateScore(0);
}

void UGameWidget::UpdateScore(int Score)
{
	if (!ScoreText) return;

	FString ScoreString = FString::Printf(TEXT("Score: %d"), Score);
	ScoreText->SetText(FText::FromString(ScoreString));
}

void UGameWidget::UpdateTimer(int Time)
{
	if (!TimerText) return;

	FString TimerString = FString::Printf(TEXT("Time: %d"), Time);
	TimerText->SetText(FText::FromString(TimerString));
}
