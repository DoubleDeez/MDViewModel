#pragma once

#include "MDViewModelWidgetExtension.h"
#include "Extensions/WidgetBlueprintGeneratedClassExtension.h"
#include "Interfaces/MDVMCompiledAssignmentsInterface.h"
#include "Util/MDViewModelAssignment.h"
#include "Util/MDViewModelAssignmentData.h"
#include "MDViewModelWidgetClassExtension.generated.h"


/**
 *
 */
UCLASS()
class MDVIEWMODEL_API UMDViewModelWidgetClassExtension : public UWidgetBlueprintGeneratedClassExtension, public IMDVMCompiledAssignmentsInterface
{
	GENERATED_BODY()

public:
	virtual void BeginDestroy() override;
	virtual void Initialize(UUserWidget* UserWidget) override;

#if WITH_EDITOR
	virtual void Construct(UUserWidget* UserWidget) override;
#endif

	virtual void SetAssignments(const TMap<FMDViewModelAssignment, FMDViewModelAssignmentData>& InAssignments) override;
	virtual const TMap<FMDViewModelAssignment, FMDViewModelAssignmentData>& GetAssignments() const override { return Assignments; }

	// Used to listen for changes before the specified widget's viewmodel extension is created
	void QueueListenForChanges(UUserWidget* Widget, FMDVMOnViewModelSet::FDelegate&& Delegate, const FMDViewModelAssignmentReference& Assignment);

protected:
	UPROPERTY()
	TMap<FMDViewModelAssignment, FMDViewModelAssignmentData> Assignments;

private:
	struct QueuedListenerData
	{
		FMDVMOnViewModelSet::FDelegate Delegate;
		FMDViewModelAssignmentReference Assignment;
	};

	TMap<TWeakObjectPtr<UUserWidget>, TArray<QueuedListenerData>> QueuedDelegates;

};
