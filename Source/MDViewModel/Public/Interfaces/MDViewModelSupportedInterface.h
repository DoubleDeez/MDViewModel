#pragma once

#include "MDViewModelRuntimeInterface.h"
#include "MDVMCompiledAssignmentsInterface.h"
#include "UObject/Interface.h"
#include "MDViewModelSupportedInterface.generated.h"

/*
 * Add view model support to any blueprintable object, also requires IMDVMCompiledAssignmentsInterface
 *
 * Usage example:
 *
 *  UCLASS(Blueprintable)
 *  class UMyClassWithViewModels : public UObject, public IMDViewModelSupportedInterface, public IMDVMCompiledAssignmentsInterface
 *  {
 *		GENERATED_BODY()
 *
 *	public:
 *		// Something like BeginPlay
 *		void SomeInitializeFunction()
 *		{
 *			InitializeViewModelSupport();
 *		}
 *
 *		virtual void BeginDestroy() override
 *		{
 *			OnBeginDestroy.Broadcast();
 *
 *			Super::BeginDestroy();
 *		}
 *
 *		virtual void SetAssignments(const TMap<FMDViewModelAssignment, FMDViewModelAssignmentData>& InAssignments) override { Assignments = InAssignments; }
 *		virtual const TMap<FMDViewModelAssignment, FMDViewModelAssignmentData>& GetAssignments() const override { return Assignments; }
 *
 *	protected:
 *		virtual TMap<FMDViewModelAssignmentReference, TObjectPtr<UMDViewModelBase>>& GetViewModels() override { return ViewModels; }
 *
 *	private:
 *		// The compiled view model assignments for this object, only exists on the CDO
 *		UPROPERTY(DuplicateTransient)
 *		TMap<FMDViewModelAssignment, FMDViewModelAssignmentData> Assignments;
 *
 *		// The runtime view model instances for this object
 *		UPROPERTY(Transient)
 *		TMap<FMDViewModelAssignmentReference, TObjectPtr<UMDViewModelBase>> ViewModels;
 *  };
 *
 */

UINTERFACE(meta=(CannotImplementInterfaceInBlueprint))
class MDVIEWMODEL_API UMDViewModelSupportedInterface : public UMDViewModelRuntimeInterface
{
	GENERATED_BODY()
};

/**
 * Interface that adds view model support to Blueprints of classes that implement it
 */
class MDVIEWMODEL_API IMDViewModelSupportedInterface : public IMDViewModelRuntimeInterface
{
	GENERATED_BODY()

public:
	void InitializeViewModelSupport();

	virtual UObject* GetOwningObject() const override;

	virtual UGameInstance* ResolveGameInstance() const override;
	virtual UWorld* ResolveWorld() const override;
	virtual ULocalPlayer* ResolveOwningLocalPlayer() const override;
	virtual APlayerController* ResolveOwningPlayer() const override;

protected:
	virtual TMap<FMDViewModelAssignmentReference, TObjectPtr<UMDViewModelBase>>& GetViewModels() override = 0;

};
