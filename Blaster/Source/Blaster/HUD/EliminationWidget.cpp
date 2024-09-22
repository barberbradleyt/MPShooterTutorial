// Fill out your copyright notice in the Description page of Project Settings.

#include "EliminationWidget.h"
#include "Components/TextBlock.h"
#include "Components/HorizontalBox.h"

void UEliminationWidget::SetEliminationAnnouncementText(FString AttackerName, FString VictimName)
{
	FString ElimAnnoucementText = FString::Printf(TEXT("%s eliminated %s!"), *AttackerName, *VictimName);
	if (AnnouncementText)
	{
		AnnouncementText->SetText(FText::FromString(ElimAnnoucementText));
	}
}