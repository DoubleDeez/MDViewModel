#pragma once

#include "Components/Widget.h"
#include "Util/MDViewModelAssignmentReference.h"
#include "ViewModel/MDViewModelBase.h"
#include "Widgets/Input/SComboBox.h"
#include "MDViewModelComboBox.generated.h"

/**
 * Combo box widget that can be populated by a list of view models
 */
UCLASS(DisplayName="ComboBox (View Model)")
class MDVIEWMODEL_API UMDViewModelComboBox : public UWidget
{
	GENERATED_BODY()

public:
	UMDViewModelComboBox();

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemSelectedDynamic, UMDViewModelBase*, Item);
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnItemSelected, UMDViewModelBase*);

	UFUNCTION(BlueprintCallable , Category = "Combo Box")
	void PopulateItems(const TArray<UMDViewModelBase*>& InViewModels);

	// 'Setter' functions require UFUNCTION markup until 5.2
	UFUNCTION()
	void SetSelectedItem(UMDViewModelBase* ViewModel);

	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

	virtual void PostInitProperties() override;

#if WITH_EDITOR
	virtual const FText GetPaletteCategory() override;
	virtual void ValidateCompiledDefaults(class IWidgetCompilerLog& CompileLog) const override;
#endif

	UPROPERTY(EditAnywhere, Category = "Combo Box")
	TSubclassOf<UUserWidget> EntryWidgetClass;

	UPROPERTY(EditAnywhere, Category = "Combo Box")
	FMDViewModelAssignmentReference ViewModelAssignment;

	UPROPERTY(EditAnywhere, Category = "Interaction")
	bool bIsFocusable = true;

	UPROPERTY(EditAnywhere, Category = "Style", meta=(DisplayName="Style", DesignerRebuild))
	FComboBoxStyle WidgetStyle;

	UPROPERTY(EditAnywhere, Category = "Style", meta=(DesignerRebuild))
	FTableRowStyle EntryStyle;

	UPROPERTY(EditAnywhere, Category = "Style", meta=(DesignerRebuild))
	FScrollBarStyle ScrollBarStyle;

	UPROPERTY(BlueprintAssignable, DisplayName = "On Item Selected", Category = "Combo Box")
	FOnItemSelectedDynamic BP_OnItemSelected;
	FOnItemSelected OnItemSelected;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

	TSharedRef<SWidget> GenerateEntryWidget(UMDViewModelBase* Item) const;
	void OnSelectionChanged(UMDViewModelBase* Item, ESelectInfo::Type SelectType);

	void UpdateSelectedItemWidget();

	UClass* GetEntryWidgetClass() const;

	TSharedPtr<SComboBox<UMDViewModelBase*>> MyComboBox;
	TSharedPtr<SBox> ComboBoxContent;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Setter, Transient,  Category = "Combo Box")
	TObjectPtr<UMDViewModelBase> SelectedItem;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMDViewModelBase>> ViewModels;
};
