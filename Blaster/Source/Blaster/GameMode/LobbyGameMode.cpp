// Fill out your copyright notice in the Description page of Project Settings.


#include "LobbyGameMode.h"
#include "GameFramework/GameStateBase.h"
#include "MultiplayerSessionsSubsystem.h"

void ALobbyGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	int32 NumberOfPlayers = GameState.Get()->PlayerArray.Num();

	UGameInstance* GameInstance = GetGameInstance();
	if (GameInstance)
	{
		UMultiplayerSessionsSubsystem* Subsystem = GameInstance->GetSubsystem<UMultiplayerSessionsSubsystem>();
		check(Subsystem);

		FString* LobbyCode = Subsystem->DesiredMatchProperties.Find(FName("LobbyCode"));
		if (LobbyCode) {
			UE_LOG(LogTemp, Warning, TEXT("LobbyCode=%s"), **LobbyCode);
		}

		if (NumberOfPlayers == Subsystem->DesiredNumPublicConnections)
		{
			UWorld* World = GetWorld();
			if (World)
			{
				bUseSeamlessTravel = true;

				TMap<FName, FString> MatchProperties = Subsystem->DesiredMatchProperties;
				FString MatchType = *MatchProperties.Find(MATCH_TYPE_PROPERTY);
				if (MatchType == "FreeForAll")
				{
					World->SeamlessTravel(FString("/Game/Maps/BlasterMap?listen"), true);
				}
				else if (MatchType == "TeamDeathMatch")
				{
					World->SeamlessTravel(FString("/Game/Maps/TeamDeathMatchMap?listen"), true);
				}
				else if (MatchType == "CaptureTheFlag")
				{
					World->SeamlessTravel(FString("/Game/Maps/CaptureTheFlagMap?listen"), true);
				}
			}
		}
	}
}