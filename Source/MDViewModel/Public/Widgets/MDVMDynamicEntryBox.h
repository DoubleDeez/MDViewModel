#pragma once

#include "Components/DynamicEntryBox.h"
#include "Util/MDViewModelAssignmentReference.h"
#include "MDVMDynamicEntryBox.generated.h"

class UMDViewModelBase;

/**
 * A dynamic entry box that can be populated by a list of view models
 */
UCLASS(meta = (DisplayName = "Dynamic Entry Box (View Model)"))
class MDVIEWMODEL_API UMDVMDynamicEntryBox : public UDynamicEntryBox
{
	GENERATED_BODY()

public:
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnEntryDynamicEvent, UUserWidget*, Widget, UMDViewModelBase*, ViewModel);

	virtual void PostInitProperties() override;
	virtual TSharedRef<SWidget> RebuildWidget() override;

#if WITH_EDITOR
	virtual const FText GetPaletteCategory() override;
	virtual void ValidateCompiledDefaults(class IWidgetCompilerLog& CompileLog) const override;
#endif

	UFUNCTION(BlueprintCallable, Category = "DynamicEntryBox")
	void PopulateItems(const TArray<UMDViewModelBase*>& ViewModels);

	// Event triggered for each entry when PopulateItems is called, before Construct is called on new entries.
	UPROPERTY(BlueprintAssignable, Category = "DynamicEntryBox")
	FOnEntryDynamicEvent OnEntryGenerated;

	// Event triggered right before removing entries in PopulateItems, usually due to a smaller list being passed in than was last populated.
	UPROPERTY(BlueprintAssignable, Category = "DynamicEntryBox")
	FOnEntryDynamicEvent OnEntryRemoved;

protected:
	virtual void SynchronizeProperties() override;
	virtual void AddEntryChild(UUserWidget& ChildWidget) override;

	// The view model to set on the entry widgets when populating this list
	UPROPERTY(EditAnywhere, Category = "EntryLayout", meta = (DisplayAfter = "EntryWidgetClass"))
	FMDViewModelAssignmentReference ViewModelAssignment;

	// At least this many widgets will display when calling PopulateItems, setting null view models to make up the difference in items if necessary
	// Also acts as a lower limit for `NumDesignerPreviewEntries`
	UPROPERTY(EditAnywhere, Category = "DynamicEntryBox", meta = (DisplayAfter = "NumDesignerPreviewEntries", ClampMin = 0))
	int32 MinimumEntriesToDisplay = 0;

private:
#if WITH_EDITOR
	UClass* GetEditorTimeEntryWidgetClass() const;
#endif

	void PopulateEntryWidget(UUserWidget* EntryWidget) const;

	// The current view model being constructed, for injecting set view models before Entry Widget construction in AddEntryChild
	TWeakObjectPtr<UMDViewModelBase> CurrentViewModel;

	// View models that were populated before the slate widget was constructed
	TArray<TWeakObjectPtr<UMDViewModelBase>> PendingViewModels;
	bool bUsePendingList = false;

};
