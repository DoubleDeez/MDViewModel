#pragma once

#include "MDViewModelWidgetExtension.h"
#include "Extensions/WidgetBlueprintGeneratedClassExtension.h"
#include "Util/MDViewModelAssignmentData.h"
#include "MDViewModelWidgetClassExtension.generated.h"


/**
 *
 */
UCLASS()
class MDVIEWMODEL_API UMDViewModelWidgetClassExtension : public UWidgetBlueprintGeneratedClassExtension
{
	GENERATED_BODY()

public:
	virtual void Initialize(UUserWidget* UserWidget) override;

#if WITH_EDITOR
	virtual void Construct(UUserWidget* UserWidget) override;
#endif

	void SetAssignments(const TMap<FMDViewModelAssignment, FMDViewModelAssignmentData>& InAssignments);

	const TMap<FMDViewModelAssignment, FMDViewModelAssignmentData>& GetAssignments() const { return Assignments; }
	void GetThisAndAncestorAssignments(TMap<FMDViewModelAssignment, FMDViewModelAssignmentData>& OutAssignments) const;
	void SearchAssignments(TMap<FMDViewModelAssignment, FMDViewModelAssignmentData>& OutViewModelAssignments, TSubclassOf<UMDViewModelBase> ViewModelClass = nullptr, const FGameplayTag& ProviderTag = FGameplayTag::EmptyTag, const FName& ViewModelName = NAME_None) const;

	void GetViewModelClasses(TSet<TSubclassOf<UMDViewModelBase>>& OutViewModelClasses) const;

	bool HasAssignments() const { return !Assignments.IsEmpty(); }

	// Used to listen for changes before the specified widget's viewmodel extension is created
	void QueueListenForChanges(UUserWidget* Widget, FMDVMOnViewModelSet::FDelegate&& Delegate, TSubclassOf<UMDViewModelBase> ViewModelClass, FName ViewModelName = MDViewModelUtils::DefaultViewModelName);

protected:
	UPROPERTY()
	TMap<FMDViewModelAssignment, FMDViewModelAssignmentData> Assignments;

private:
	UPROPERTY(Transient)
	mutable TMap<FMDViewModelAssignment, FMDViewModelAssignmentData> ParentAssignments;
	
	UPROPERTY(Transient)
	mutable bool bHasGatheredParentAssignments = false;

	struct QueuedListenerData
	{
		FMDVMOnViewModelSet::FDelegate Delegate;
		TSubclassOf<UMDViewModelBase> ViewModelClass;
		FName ViewModelName = NAME_None;
	};

	TMap<TWeakObjectPtr<UUserWidget>, TArray<QueuedListenerData>> QueuedDelegates;

};
