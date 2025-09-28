// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSessionSettings.h"

#include "Menu.generated.h"

USTRUCT(BlueprintType)
struct FSessionResult
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadOnly)
	FString SessionIdStr;

	UPROPERTY(BlueprintReadOnly)
	FString OwnerUserName;

	UPROPERTY(BlueprintReadOnly)
	int32 Ping;

	UPROPERTY(BlueprintReadOnly)
	int32 OpenPublicConnections;

	UPROPERTY(BlueprintReadOnly)
	int32 TotalPublicConnections;
};

/**
 * 
 */
UCLASS()
class MULTIPLAYERSESSIONS_API UMenu : public UUserWidget
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable)
	void MenuSetup(
		TMap<FName, FString> MatchProperties,
		TMap<FName, FString> DefaultSearchProperties,
		int32 NumberOfPublicConnections = 4,
		bool IsPrivate = false,
		FString LobbyPath = FString(TEXT("/Game/ThirdPerson/Maps/Lobby")) //TODO: Do we like this?
	);

protected:
	virtual bool Initialize() override;
	virtual void NativeDestruct() override;

	UPROPERTY(BlueprintReadOnly)
	TArray<FSessionResult> DisplaySessionResults;

	//
	// Callbacks for the custom delegates on MultiplayerSessionsSubsystem
	//
	UFUNCTION()
	void OnCreateSession(bool bWasSuccessful);
	void OnFindSessions(const TArray<FOnlineSessionSearchResult>& SessionResults, bool bWasSuccessful);
	void OnJoinSession(EOnJoinSessionCompleteResult::Type Result);
	UFUNCTION()
	void OnDestroySession(bool bWasSuccessful);
	UFUNCTION()
	void OnStartSession(bool bWasSuccessful);

	//
	// Blueprint compatable callbacks for the custom delegates on MultiplayerSessionsSubsystem
	//
	UFUNCTION(BlueprintImplementableEvent)
	void OnCreateSession_BP(bool bWasSuccessful);
	UFUNCTION(BlueprintImplementableEvent)
	void OnFindSessions_BP(bool bWasSuccessful);
	UFUNCTION(BlueprintImplementableEvent)
	void OnJoinSession_BP(const FString& Result);

	UFUNCTION(BlueprintCallable)
	void HostSession();

	UFUNCTION(BlueprintCallable)
	void FindSessions();

	UFUNCTION(BlueprintCallable)
	void FindSessionsByProperties(const TMap<FName, FString> SearchProperties);

	UFUNCTION(BlueprintCallable)
	void JoinSessionByIdStr(const FString& SessionIdStr);

	UFUNCTION(BlueprintImplementableEvent)
	void OnSessionResultsAvailable();

private:
	int MaxSearchResults = 10;
	TMap<FString, FOnlineSessionSearchResult> LastFindSessionResults;

	void JoinSession(FOnlineSessionSearchResult* SessionToJoin);

	void MenuTearDown();

	// Subsystem designed to handle all online session functionality
	class UMultiplayerSessionsSubsystem* MultiplayerSessionsSubsystem;

	// Set only via constructor
	// For game-wide, general search properties
	// i.e. build number
	TMap<FName, FString> DefaultSearchProperties;

	// Can be set per-match
	UPROPERTY(BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	int32 NumPublicConnections{4};
	UPROPERTY(BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	TMap<FName, FString> MatchProperties;
	UPROPERTY(BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	bool IsPrivate;

	FString PathToLobby{TEXT("")};
};
