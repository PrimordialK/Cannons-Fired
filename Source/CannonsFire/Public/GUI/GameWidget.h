// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "GameWidget.generated.h"

// File-scope delegate declaration
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTimerEnd);

UCLASS()
class CANNONSFIRE_API UGameWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	// Use NativeTick to drive the countdown
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// Bound UMG widgets
	UPROPERTY(meta = (BindWidget))
	UTextBlock* TimerText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* ScoreText;

	// Countdown timer state
	UPROPERTY(VisibleAnywhere, Category = "Timer")
	int32 RemainingTime = 60;

	// Broadcast when timer reaches zero (bindable in C++/Blueprint)
	UPROPERTY(BlueprintAssignable, Category = "Timer")
	FOnTimerEnd OnTimerEnd;

	// Start/stop the countdown (called by HUD on BeginPlay)
	UFUNCTION(BlueprintCallable, Category = "Timer")
	void StartTimer(int32 StartTime = 60);

	UFUNCTION()
	void StopTimer();

	// UI helpers
	UFUNCTION()
	void UpdateScore(int Score);

	UFUNCTION()
	void UpdateTimer(int Time);

private:
	// Accumulates delta seconds; when >= 1.0 reduce RemainingTime by one
	float AccumulatedDelta = 0.0f;

	// Whether the countdown is running
	bool bTimerRunning = false;
};
