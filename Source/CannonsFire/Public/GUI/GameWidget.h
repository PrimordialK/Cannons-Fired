// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "GameWidget.generated.h"

/**
 * 
 */
UCLASS()
class CANNONSFIRE_API UGameWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* TimerText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* ScoreText;

	UFUNCTION()
	void UpdateScore(int Score);

	UFUNCTION()
	void UpdateTimer(int Time);


	
};
