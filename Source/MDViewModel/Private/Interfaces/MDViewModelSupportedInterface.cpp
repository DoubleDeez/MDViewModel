#include "Interfaces/MDViewModelSupportedInterface.h"

#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"

void IMDViewModelSupportedInterface::InitializeViewModelSupport()
{
	UBlueprintGeneratedClass::BindDynamicDelegates(GetOwningObjectClass(), GetOwningObject());
}

UObject* IMDViewModelSupportedInterface::GetOwningObject() const
{
	return Cast<UObject>(const_cast<IMDViewModelSupportedInterface*>(this));
}

UGameInstance* IMDViewModelSupportedInterface::ResolveGameInstance() const
{
	if (const UWorld* World = ResolveWorld())
	{
		return World->GetGameInstance();
	}

	return nullptr;
}

UWorld* IMDViewModelSupportedInterface::ResolveWorld() const
{
	return GetOwningObject()->GetWorld();
}

ULocalPlayer* IMDViewModelSupportedInterface::ResolveOwningLocalPlayer() const
{
	if (ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(GetOwningObject()))
	{
		return LocalPlayer;
	}

	if (const APlayerController* PC = ResolveOwningPlayer())
	{
		return PC->GetLocalPlayer();
	}

	return nullptr;
}

APlayerController* IMDViewModelSupportedInterface::ResolveOwningPlayer() const
{
	if (APlayerController* PC = Cast<APlayerController>(GetOwningObject()))
	{
		return PC;
	}

	if (const ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(GetOwningObject()))
	{
		return LocalPlayer->GetPlayerController(ResolveWorld());
	}

	if (const APlayerState* PS = Cast<APlayerState>(GetOwningObject()))
	{
		return PS->GetPlayerController();
	}

	if (const APawn* Pawn = Cast<APawn>(GetOwningObject()))
	{
		return Pawn->GetController<APlayerController>();
	}

	return nullptr;
}
