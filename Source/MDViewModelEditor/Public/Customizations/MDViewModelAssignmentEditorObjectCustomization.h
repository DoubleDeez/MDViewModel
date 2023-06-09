#pragma once

#include "GameplayTagContainer.h"
#include "IDetailCustomization.h"
#include "Input/Reply.h"


class UMDViewModelAssignmentEditorObject;
class SWidget;
class SMDViewModelAssignmentDialog;

/**
 * Customization for UMDViewModelAssignmentEditorObject
 */
class MDVIEWMODELEDITOR_API FMDViewModelAssignmentEditorObjectCustomization : public IDetailCustomization
{
public:
	static TSharedRef<IDetailCustomization> MakeInstance(TSharedRef<SMDViewModelAssignmentDialog> Dialog, bool bIsEditMode)
	{
		return MakeShared<FMDViewModelAssignmentEditorObjectCustomization>(Dialog, bIsEditMode);
	}

	FMDViewModelAssignmentEditorObjectCustomization(TSharedRef<SMDViewModelAssignmentDialog> InDialog, bool bIsEditMode);

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

	void RefreshDetails() const;

	void OnProviderPropertyChanged() const;
	void OnViewModelPropertyChanged() const;

	TWeakPtr<IDetailLayoutBuilder> CachedDetailBuilder;
	TWeakObjectPtr<UMDViewModelAssignmentEditorObject> EditorObjectPtr;
	TSharedRef<SMDViewModelAssignmentDialog> Dialog;
	bool bIsEditMode;
};