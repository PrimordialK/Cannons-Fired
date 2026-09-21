// Fill out your copyright notice in the Description page of Project Settings.

#include "GUI/GameWidget.h"
#include "Engine/Engine.h"

void UGameWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// initialize UI
	UpdateTimer(RemainingTime);
	UpdateScore(0);

	// start countdown immediately using the world timer (robust)
	StartTimer(RemainingTime);
}

void UGameWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!bTimerRunning) return;

	AccumulatedDelta += InDeltaTime;
	// handle cases where more than 1 second elapsed (e.g., hitches)
	while (AccumulatedDelta >= 1.0f && bTimerRunning)
	{
		AccumulatedDelta -= 1.0f;

		if (RemainingTime <= 0)
		{
			// already at zero: ensure we stop and broadcast once
			StopTimer();
			UpdateTimer(0);
			OnTimerEnd.Broadcast();
			break;
		}

		--RemainingTime;
		UpdateTimer(RemainingTime);

		if (RemainingTime <= 0)
		{
			StopTimer();
			OnTimerEnd.Broadcast();
			break;
		}
	}
}

void UGameWidget::StartTimer(int32 StartTime)
{
	if (StartTime < 0) StartTime = 0;
	RemainingTime = StartTime;
	AccumulatedDelta = 0.0f;
	UpdateTimer(RemainingTime);
	bTimerRunning = true;
}

void UGameWidget::StopTimer()
{
	bTimerRunning = false;
	AccumulatedDelta = 0.0f;
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
