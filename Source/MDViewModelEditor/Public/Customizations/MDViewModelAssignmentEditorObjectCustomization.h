#pragma once

#include "GameplayTagContainer.h"
#include "IDetailCustomization.h"
#include "Input/Reply.h"
#include "Layout/Visibility.h"


class UMDViewModelAssignmentEditorObject;
class SWidget;
class SMDViewModelAssignmentDialog;

/**
 * Customization for UMDViewModelAssignmentEditorObject
 */
class MDVIEWMODELEDITOR_API FMDViewModelAssignmentEditorObjectCustomization : public IDetailCustomization
{
public:
	static TSharedRef<IDetailCustomization> MakeInstance(TSharedRef<SMDViewModelAssignmentDialog> Dialog)
	{
		return MakeShared<FMDViewModelAssignmentEditorObjectCustomization>(Dialog);
	}

	FMDViewModelAssignmentEditorObjectCustomization(TSharedRef<SMDViewModelAssignmentDialog> InDialog);

	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

	virtual void CustomizeDetails(const TSharedPtr<IDetailLayoutBuilder>& DetailBuilder) override
	{
		CachedDetailBuilder = DetailBuilder;
		CustomizeDetails(*DetailBuilder);
	}

private:
	FText GetSelectedProviderName() const;

	FText GetSelectedProviderToolTip() const;

	TSharedRef<SWidget> OnGetProviderMenuContent() const;

	void OnProviderSelected(FGameplayTag Tag) const;

	void OnViewModelClassPicked(UClass* ViewModelClass) const;

	UClass* GetCurrentViewModelClass() const;

	FReply OnClassPickerButtonClicked() const;

	FText GetSelectedClassText() const;

	FText GetSelectedClassToolTipText() const;

	void OnConfigPropertyChanged(const FPropertyChangedEvent& Event) const;

	void RefreshDetails() const;

	void OnProviderPropertyChanged() const;
	void OnViewModelPropertyChanged() const;

	void OnAssignmentUpdated() const;

	EVisibility GetViewModelNameVisibility() const;
	EVisibility GetViewModelTagVisibility() const;

	TWeakPtr<IDetailLayoutBuilder> CachedDetailBuilder;
	TWeakObjectPtr<UMDViewModelAssignmentEditorObject> EditorObjectPtr;
	TSharedRef<SMDViewModelAssignmentDialog> Dialog;
};