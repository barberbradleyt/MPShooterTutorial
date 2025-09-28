// Fill out your copyright notice in the Description page of Project Settings.


#include "Menu.h"
#include "MultiplayerSessionsSubsystem.h"
#include "OnlineSubsystem.h"

void UMenu::MenuSetup(TMap<FName, FString> InMatchProperties, TMap<FName, FString> InDefaultSearchProperties, int32 NumberOfPublicConnections, bool InIsPrivate, FString LobbyPath)
{
	PathToLobby = FString::Printf(TEXT("%s?listen"), *LobbyPath);
	NumPublicConnections = NumberOfPublicConnections;
	MatchProperties = InMatchProperties;
	DefaultSearchProperties = InDefaultSearchProperties;
	IsPrivate = InIsPrivate;
	AddToViewport();
	SetVisibility(ESlateVisibility::Visible);
	SetIsFocusable(true);

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

	LastFindSessionResults = TMap<FString, FOnlineSessionSearchResult>();
	DisplaySessionResults = TArray<FSessionResult>();

	return true;
}

void UMenu::OnCreateSession(bool bWasSuccessful)
{
	OnCreateSession_BP(bWasSuccessful);
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
	bool Debugging = false;

	OnFindSessions_BP(bWasSuccessful);
	if (!Debugging && (!bWasSuccessful || SessionResults.Num() <= 0))
	{

		if (!bWasSuccessful)
		{
			UE_LOG(LogTemp, Error, TEXT("Menu::OnFindSessions - bWasSuccessful=false; Found %d sessions"), SessionResults.Num());
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Menu::OnFindSessions - bWasSuccessful=true; Found %d sessions"), SessionResults.Num());
		}

		return;
	}

	LastFindSessionResults.Empty();
	DisplaySessionResults.Empty();
	for (auto Result : SessionResults) {
		if (Result.IsValid()) {

			FString ResultIdStr = Result.GetSessionIdStr();
			UE_LOG(LogTemp, Warning, TEXT("Adding ResultIdStr %s to LastFindSessionResults Map"), *ResultIdStr);
			LastFindSessionResults.Add(Result.GetSessionIdStr(), Result);

			FSessionResult SessionResult = FSessionResult();
			SessionResult.SessionIdStr = Result.GetSessionIdStr();
			SessionResult.OwnerUserName = Result.Session.OwningUserName;
			SessionResult.Ping = Result.PingInMs;
			SessionResult.OpenPublicConnections = Result.Session.NumOpenPublicConnections;
			SessionResult.TotalPublicConnections = Result.Session.SessionSettings.NumPublicConnections;
			DisplaySessionResults.Add(SessionResult);
		}
	}
	TArray<FString> Keys;
	LastFindSessionResults.GetKeys(Keys);
	 
	// debugging 
	if (Debugging)
	{
		int DebugSessionsToAdd = 4;

		for (int i = 0; i < DebugSessionsToAdd; i++) {
			FSessionResult SessionResult = FSessionResult();
			FString iStr = FString::FormatAsNumber(i);
			SessionResult.SessionIdStr = "Test_" + iStr;
			SessionResult.OwnerUserName = "TestOwner_" + iStr;
			SessionResult.OpenPublicConnections = 2;
			SessionResult.TotalPublicConnections = 4;
			SessionResult.Ping = 100;
			DisplaySessionResults.Add(SessionResult);
		}
	}

	// Notify implementing Blueprint classes that the search results are available to display
	OnSessionResultsAvailable();
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
		OnJoinSession_BP(ResultString);
		UE_LOG(LogTemp, Error, TEXT("Result != EOnJoinSessionCompleteResult::Success; Result=%s"), *ResultString);

		return;
	}
	OnJoinSession_BP("Success");

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

void UMenu::JoinSessionByIdStr(const FString& SessionIdStr) {

	TArray<FString> Keys;
	LastFindSessionResults.GetKeys(Keys);

	UE_LOG(LogTemp, Warning, TEXT("Joining Session By ID Str: %s"), *SessionIdStr);
	if (LastFindSessionResults.Contains(*SessionIdStr)) {
		FOnlineSessionSearchResult* SessionToJoin = LastFindSessionResults.Find(*SessionIdStr);

		FString SessionIdStr = SessionToJoin->GetSessionIdStr();
		UE_LOG(LogTemp, Warning, TEXT("Found Session: %s, joining..."), &SessionIdStr);

		JoinSession(SessionToJoin);
	}
	else {
		UE_LOG(LogTemp, Warning, TEXT("No Session found for ID Str: %s"), *SessionIdStr);
	}
}

void UMenu::HostSession()
{
	if (MultiplayerSessionsSubsystem)
	{
		MultiplayerSessionsSubsystem->CreateSession(NumPublicConnections, MatchProperties, IsPrivate);
	}
}

void UMenu::FindSessions()
{
	if (MultiplayerSessionsSubsystem)
	{
		MultiplayerSessionsSubsystem->FindSessions(MaxSearchResults, DefaultSearchProperties);
	}
}

void UMenu::FindSessionsByProperties(const TMap<FName, FString> SearchProperties)
{
	if (MultiplayerSessionsSubsystem)
	{
		TMap<FName, FString> AllSearchProperties = TMap<FName, FString>();
		for (TPair<FName, FString>& Property : DefaultSearchProperties) {
			AllSearchProperties.Add(Property);
		}
		for (TPair<FName, FString>& Property : SearchProperties.Array()) {
			AllSearchProperties.Add(Property);
		}
		MultiplayerSessionsSubsystem->FindSessions(MaxSearchResults, AllSearchProperties);
	}
}

void UMenu::JoinSession(FOnlineSessionSearchResult* SessionToJoin)
{
	if (MultiplayerSessionsSubsystem)
	{
		MultiplayerSessionsSubsystem->JoinSession(*SessionToJoin);
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