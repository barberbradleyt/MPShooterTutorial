// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "EliminationWidget.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API UEliminationWidget : public UUserWidget
{
	GENERATED_BODY()
public:

	void SetEliminationAnnouncementText(FString AttackerName, FString VictimName);


	UPROPERTY(meta = (BindWidget))
	class UHorizontalBox* AnnouncementBox;

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* AnnouncementText;
};
