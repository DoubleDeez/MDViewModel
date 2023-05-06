#pragma once

#include "CoreMinimal.h"
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

	void SetAssignments(const TMap<FMDViewModelAssignment, FMDViewModelAssignmentData>& InAssignments);

	const TMap<FMDViewModelAssignment, FMDViewModelAssignmentData>& GetAssignments() const { return Assignments; }
	void SearchAssignments(TMap<FMDViewModelAssignment, FMDViewModelAssignmentData>& OutViewModelAssignments, TSubclassOf<UMDViewModelBase> ViewModelClass = nullptr, const FGameplayTag& ProviderTag = FGameplayTag::EmptyTag, const FName& ViewModelName = NAME_None) const;

	void GetViewModelClasses(TSet<TSubclassOf<UMDViewModelBase>>& OutViewModelClasses) const;

	bool HasAssignments() const { return !Assignments.IsEmpty(); }

protected:
	UPROPERTY()
	TMap<FMDViewModelAssignment, FMDViewModelAssignmentData> Assignments;

private:
	void GatherParentAssignments(TSubclassOf<UUserWidget> WidgetClass);

	UPROPERTY(Transient)
	bool bHasGatheredParentAssignments = false;

};
