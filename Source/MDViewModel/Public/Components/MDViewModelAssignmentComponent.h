#pragma once

#include "Components/ActorComponent.h"
#include "Interfaces/MDVMCompiledAssignmentsInterface.h"
#include "Interfaces/MDViewModelRuntimeInterface.h"
#include "Util/MDViewModelAssignment.h"
#include "Util/MDViewModelAssignmentData.h"
#include "Util/MDViewModelAssignmentReference.h"
#include "MDViewModelAssignmentComponent.generated.h"

class UBlueprintGeneratedClass;

// Auto-generated component to track view models on an actor
UCLASS(Hidden, meta=(BlueprintSpawnableComponent))
class MDVIEWMODEL_API UMDViewModelAssignmentComponent : public UActorComponent, public IMDVMCompiledAssignmentsInterface, public IMDViewModelRuntimeInterface
{
	GENERATED_BODY()

public:
	virtual void BeginDestroy() override;
	
	virtual UObject* GetOwningObject() const override;

	virtual UGameInstance* ResolveGameInstance() const override;
	virtual UWorld* ResolveWorld() const override;
	virtual ULocalPlayer* ResolveOwningLocalPlayer() const override;
	virtual APlayerController* ResolveOwningPlayer() const override;
	
	void SetAssignments(const TMap<FMDViewModelAssignment, FMDViewModelAssignmentData>& InAssignments);
	virtual const TMap<FMDViewModelAssignment, FMDViewModelAssignmentData>& GetAssignments() const override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual TMap<FMDViewModelAssignmentReference, TObjectPtr<UMDViewModelBase>>& GetViewModels() override;
	virtual const TMap<FMDViewModelAssignmentReference, TObjectPtr<UMDViewModelBase>>& GetViewModels() const override;

	// TODO - Details customization to hide all properties and add a button to open view model tab

private:
	static void BindDelegates(IMDViewModelRuntimeInterface& Object, UBlueprintGeneratedClass* Class);
	static void UnbindDelegates(IMDViewModelRuntimeInterface& Object, UBlueprintGeneratedClass* Class);

	// The compiled assignments for the owning actor
	UPROPERTY(DuplicateTransient)
	TMap<FMDViewModelAssignment, FMDViewModelAssignmentData> Assignments;

	// The runtime view model instances for the owning actor
	UPROPERTY(Transient)
	TMap<FMDViewModelAssignmentReference, TObjectPtr<UMDViewModelBase>> ViewModels;
};
