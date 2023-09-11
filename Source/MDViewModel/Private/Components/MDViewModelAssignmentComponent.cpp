#include "Components/MDViewModelAssignmentComponent.h"

#include "Bindings/MDVMBlueprintBindingBase.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"

void UMDViewModelAssignmentComponent::SetAssignments(const TMap<FMDViewModelAssignment, FMDViewModelAssignmentData>& InAssignments)
{
	Assignments = InAssignments;
}

const TMap<FMDViewModelAssignment, FMDViewModelAssignmentData>& UMDViewModelAssignmentComponent::GetAssignments() const
{
	return Assignments;
}

void UMDViewModelAssignmentComponent::BeginDestroy()
{
	OnBeginDestroy.Broadcast();
	
	Super::BeginDestroy();
}

UObject* UMDViewModelAssignmentComponent::GetOwningObject() const
{
	return GetOwner();
}

UGameInstance* UMDViewModelAssignmentComponent::ResolveGameInstance() const
{
	if (const UWorld* World = GetWorld())
	{
		return World->GetGameInstance();
	}

	return nullptr;
}

UWorld* UMDViewModelAssignmentComponent::ResolveWorld() const
{
	return GetWorld();
}

ULocalPlayer* UMDViewModelAssignmentComponent::ResolveOwningLocalPlayer() const
{
	if (const APlayerController* PC = ResolveOwningPlayer())
	{
		return PC->GetLocalPlayer();
	}

	return nullptr;
}

APlayerController* UMDViewModelAssignmentComponent::ResolveOwningPlayer() const
{
	if (APlayerController* PC = GetOwner<APlayerController>())
	{
		return PC;
	}
	
	if (const APlayerState* PS = GetOwner<APlayerState>())
	{
		return PS->GetPlayerController();
	}
	
	if (const APawn* Pawn = GetOwner<APawn>())
	{
		return Pawn->GetController<APlayerController>();
	}

	return nullptr;
}

TMap<FMDViewModelAssignmentReference, TObjectPtr<UMDViewModelBase>>& UMDViewModelAssignmentComponent::GetViewModels()
{
	return ViewModels;
}

void UMDViewModelAssignmentComponent::BindDelegates(IMDViewModelRuntimeInterface& Object, UBlueprintGeneratedClass* Class)
{
	if (IsValid(Class))
	{
		for (UDynamicBlueprintBinding* Binding : Class->DynamicBindingObjects)
		{
			if (const UMDVMBlueprintBindingBase* VMBinding = Cast<UMDVMBlueprintBindingBase>(Binding))
			{
				VMBinding->BindViewModelDelegates(Object);
			}
		}

		BindDelegates(Object, Cast<UBlueprintGeneratedClass>(Class->GetSuperClass()));
	}
}

void UMDViewModelAssignmentComponent::UnbindDelegates(IMDViewModelRuntimeInterface& Object, UBlueprintGeneratedClass* Class)
{
	if (IsValid(Class))
	{
		for (UDynamicBlueprintBinding* Binding : Class->DynamicBindingObjects)
		{
			if (const UMDVMBlueprintBindingBase* VMBinding = Cast<UMDVMBlueprintBindingBase>(Binding))
			{
				VMBinding->UnbindViewModelDelegates(Object);
			}
		}

		UnbindDelegates(Object, Cast<UBlueprintGeneratedClass>(Class->GetSuperClass()));
	}
}

void UMDViewModelAssignmentComponent::BeginPlay()
{
	Super::BeginPlay();

	BindDelegates(*this, Cast<UBlueprintGeneratedClass>(GetOwner()->GetClass()));
	PopulateViewModels();
}

void UMDViewModelAssignmentComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	CleanUpViewModels();
	UnbindDelegates(*this, Cast<UBlueprintGeneratedClass>(GetOwner()->GetClass()));
	
	Super::EndPlay(EndPlayReason);
}
