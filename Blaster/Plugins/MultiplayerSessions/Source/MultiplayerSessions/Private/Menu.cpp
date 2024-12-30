// Fill out your copyright notice in the Description page of Project Settings.


#include "Menu.h"
#include "Components/Button.h"
#include "MultiplayerSessionsSubsystem.h"
#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"

void UMenu::MenuSetup(int32 NumberOfPublicConnections, FString TypeOfMatch, FString LobbyPath)
{
	PathToLobby = FString::Printf(TEXT("%s?listen"), *LobbyPath);
	NumPublicConnections = NumberOfPublicConnections;
	MatchType = TypeOfMatch;
	AddToViewport();
	SetVisibility(ESlateVisibility::Visible);
	bIsFocusable = true;

	UWorld* World = GetWorld();
	if (World)
	{
		APlayerController* PlayerController = World->GetFirstPlayerController();
		if (PlayerController)
		{
			FInputModeUIOnly InputModeData;
			InputModeData.SetWidgetToFocus(TakeWidget());
			InputModeData.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);

			PlayerController->SetInputMode(InputModeData);
			PlayerController->SetShowMouseCursor(true);
		}
	}

	UGameInstance* GameInstance = GetGameInstance();
	if (GameInstance)
	{
		MultiplayerSessionsSubsystem = GameInstance->GetSubsystem<UMultiplayerSessionsSubsystem>();
	}

	if (MultiplayerSessionsSubsystem)
	{
		MultiplayerSessionsSubsystem->MultiplayerOnCreateSessionComplete.AddDynamic(this, &ThisClass::OnCreateSession);
		MultiplayerSessionsSubsystem->MultiplayerOnFindSessionsComplete.AddUObject(this, &ThisClass::OnFindSessions);
		MultiplayerSessionsSubsystem->MultiplayerOnJoinSessionComplete.AddUObject(this, &ThisClass::OnJoinSession);
		MultiplayerSessionsSubsystem->MultiplayerOnDestroySessionComplete.AddDynamic(this, &ThisClass::OnDestroySession);
		MultiplayerSessionsSubsystem->MultiplayerOnStartSessionComplete.AddDynamic(this, &ThisClass::OnStartSession);
		MultiplayerSessionsSubsystem->DestroySession();
	}
}

bool UMenu::Initialize()
{
	if (!Super::Initialize())
	{
		return false;
	}

	if (HostButton)
	{
		HostButton->OnClicked.AddDynamic(this, &UMenu::HostButtonClicked);
	}

	if (JoinButton)
	{
		JoinButton->OnClicked.AddDynamic(this, &UMenu::JoinButtonClicked);
	}

	return true;
}

void UMenu::OnCreateSession(bool bWasSuccessful)
{
	if (bWasSuccessful)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				15.f,
				FColor::Purple,
				FString("Session created successfully!")
			);
		}

		UWorld* World = GetWorld();
		if (World)
		{
			World->ServerTravel(PathToLobby, true);
		}
	}
	else
	{
		HostButton->SetIsEnabled(true);
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				15.f,
				FColor::Red,
				FString("Session creation FAILED!")
			);
		}
	}
}

void UMenu::OnFindSessions(const TArray<FOnlineSessionSearchResult>& SessionResults, bool bWasSuccessful)
{
	if (!bWasSuccessful || SessionResults.Num() <= 0)
	{
		if (!bWasSuccessful)
		{
			UE_LOG(LogTemp, Error, TEXT("Menu::OnFindSessions - bWasSuccessful=false; Found %d session"), SessionResults.Num());
		}

		JoinButton->SetIsEnabled(true);
		return;
	}

	if (MultiplayerSessionsSubsystem == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("Menu::OnFindSessions - MultiplayerSessionsSubsystem is null"));
		return;
	}

	for (auto Result : SessionResults)
	{
		FString SettingsValue;
		Result.Session.SessionSettings.Get(FName("MatchType"), SettingsValue);

		if (SettingsValue == MatchType)
		{
			UE_LOG(LogTemp, Error, TEXT("Menu::OnFindSessions - Found session of match type '%s', joining..."), *MatchType);
			MultiplayerSessionsSubsystem->JoinSession(Result);
			return;
		}
	}
}

void UMenu::OnJoinSession(EOnJoinSessionCompleteResult::Type Result)
{
	if (Result != EOnJoinSessionCompleteResult::Success)
	{
		FString ResultString;
		switch (Result) {
		case EOnJoinSessionCompleteResult::Type::AlreadyInSession:
			ResultString = "AlreadyInSession";
			break;
		case EOnJoinSessionCompleteResult::Type::CouldNotRetrieveAddress:
			ResultString = "CouldNotRetrieveAddress";
			break;
		case EOnJoinSessionCompleteResult::Type::SessionDoesNotExist:
			ResultString = "SessionDoesNotExist";
			break;
		case EOnJoinSessionCompleteResult::Type::SessionIsFull:
			ResultString = "SessionIsFull";
			break;
		case EOnJoinSessionCompleteResult::Type::UnknownError:
		default:
			ResultString = "SessionIsFull";
			break;
		}
		UE_LOG(LogTemp, Error, TEXT("Result != EOnJoinSessionCompleteResult::Success; Result=%s"), *ResultString);

		JoinButton->SetIsEnabled(true);
	}

	IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();
	if (Subsystem)
	{
		IOnlineSessionPtr SessionInterface = Subsystem->GetSessionInterface();
		if (SessionInterface.IsValid())
		{
			FString Address;
			SessionInterface->GetResolvedConnectString(NAME_GameSession, Address);

			APlayerController* PlayerController = GetGameInstance()->GetFirstLocalPlayerController();
			if (PlayerController)
			{
				PlayerController->ClientTravel(Address, ETravelType::TRAVEL_Absolute);
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("Menu::OnJoinSession - PlayerController not valid"));
			}
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Menu::OnJoinSession - SessionInterface not valid"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Menu::OnJoinSession - OnlineSubsystem not valid"));
	}
}

void UMenu::OnDestroySession(bool bWasSuccessful)
{
	
}

void UMenu::OnStartSession(bool bWasSuccessful)
{

}

void UMenu::HostButtonClicked()
{
	// TODO: Both should be disabled/re-enabled
	HostButton->SetIsEnabled(false);
	if (MultiplayerSessionsSubsystem)
	{
		MultiplayerSessionsSubsystem->CreateSession(NumPublicConnections, MatchType);
	}
}

void UMenu::JoinButtonClicked()
{
	// TODO: Both should be disabled/re-enabled
	JoinButton->SetIsEnabled(false);
	if (MultiplayerSessionsSubsystem)
	{
		MultiplayerSessionsSubsystem->FindSessions(10000);
	}
}

void UMenu::MenuTearDown()
{
	RemoveFromParent();
	UWorld* World = GetWorld();
	if (World)
	{
		APlayerController* PlayerController = World->GetFirstPlayerController();
		if (PlayerController)
		{
			FInputModeGameOnly InputModeData;
			PlayerController->SetInputMode(InputModeData);
			PlayerController->SetShowMouseCursor(false);
		}
	}
}

void UMenu::NativeDestruct()
{
	MenuTearDown();
	Super::NativeDestruct();
}