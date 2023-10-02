#include "Widgets/MDViewModelComboBox.h"

#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Runtime/Launch/Resources/Version.h"
#include "Styling/UMGCoreStyle.h"
#include "Util/MDViewModelFunctionLibrary.h"
#include "ViewModel/MDViewModelBase.h"

#if WITH_EDITOR
#include "Editor/WidgetCompilerLog.h"
#endif

UMDViewModelComboBox::UMDViewModelComboBox()
	: WidgetStyle(FUMGCoreStyle::Get().GetWidgetStyle<FComboBoxStyle>("ComboBox"))
	, EntryStyle(FUMGCoreStyle::Get().GetWidgetStyle<FTableRowStyle>("TableView.Row"))
	, ScrollBarStyle(FUMGCoreStyle::Get().GetWidgetStyle<FScrollBarStyle>("ScrollBar"))
{
	WidgetStyle.UnlinkColors();
	EntryStyle.UnlinkColors();
	ScrollBarStyle.UnlinkColors();

	bIsFocusable = true;
	SetVisibilityInternal(ESlateVisibility::Visible);
}

void UMDViewModelComboBox::PopulateItems(const TArray<UMDViewModelBase*>& InViewModels)
{
	ViewModels = InViewModels;
	if (MyComboBox.IsValid())
	{
		MyComboBox->RefreshOptions();
	}
}

void UMDViewModelComboBox::SetSelectedItem(UMDViewModelBase* ViewModel)
{
	if (SelectedItem != ViewModel)
	{
		SelectedItem = ViewModel;
		BroadcastFieldValueChanged(FFieldNotificationClassDescriptor::SelectedItem);

		if (MyComboBox.IsValid())
		{
			MyComboBox->SetSelectedItem(ViewModel);
		}

		UpdateSelectedItemWidget();
	}
}

void UMDViewModelComboBox::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);

	MyComboBox.Reset();
	ComboBoxContent.Reset();
}

void UMDViewModelComboBox::PostInitProperties()
{
	Super::PostInitProperties();

#if WITH_EDITOR
	ViewModelAssignment.OnGetBoundObjectClass.BindUObject(this, &UMDViewModelComboBox::GetEntryWidgetClass);
#endif
}

#if WITH_EDITOR
const FText UMDViewModelComboBox::GetPaletteCategory()
{
	return INVTEXT("View Model");
}

void UMDViewModelComboBox::ValidateCompiledDefaults(IWidgetCompilerLog& CompileLog) const
{
	Super::ValidateCompiledDefaults(CompileLog);

	if (!EntryWidgetClass)
	{
		CompileLog.Error(FText::Format(INVTEXT("{0} has no EntryWidgetClass specified - required for View Model Combo Box to function."), FText::FromString(GetName())));
	}
	else if (CompileLog.GetContextClass() && EntryWidgetClass->IsChildOf(CompileLog.GetContextClass()))
	{
		CompileLog.Error(FText::Format(INVTEXT("{0} has a recursive EntryWidgetClass specified (reference itself)."), FText::FromString(GetName())));
	}
	else if (!ViewModelAssignment.IsAssignmentValid())
	{
		CompileLog.Error(FText::Format(INVTEXT("{0} needs a valid view model assignment."), FText::FromString(GetName())));
	}
}
#endif

TSharedRef<SWidget> UMDViewModelComboBox::RebuildWidget()
{
	MyComboBox =
		SNew(SComboBox<UMDViewModelBase*>)
		.OptionsSource(&ToRawPtrTArrayUnsafe(ViewModels))
		.OnGenerateWidget_UObject(this, &UMDViewModelComboBox::GenerateEntryWidget)
		.OnSelectionChanged_UObject(this, &UMDViewModelComboBox::OnSelectionChanged)
		.InitiallySelectedItem(SelectedItem)
		.IsFocusable(bIsFocusable)
		.ComboBoxStyle(&WidgetStyle)
#if ENGINE_MAJOR_VERSION > 5 || ENGINE_MINOR_VERSION >= 2
		.ScrollBarStyle(&ScrollBarStyle)
#endif
		.ItemStyle(&EntryStyle)
		[
			SAssignNew(ComboBoxContent, SBox)
		];

	UpdateSelectedItemWidget();

	return MyComboBox.ToSharedRef();
}

TSharedRef<SWidget> UMDViewModelComboBox::GenerateEntryWidget(UMDViewModelBase* Item) const
{
	// TODO - use widget pool
	if (UUserWidget* Widget = CreateWidget<UUserWidget>(Cast<UWidgetTree>(GetOuter()), EntryWidgetClass))
	{
		if (IsValid(Item))
		{
			UMDViewModelFunctionLibrary::BP_SetViewModel(Widget, Item, ViewModelAssignment);
		}

		return Widget->TakeWidget();
	}

	return SNullWidget::NullWidget;
}

void UMDViewModelComboBox::OnSelectionChanged(UMDViewModelBase* Item, ESelectInfo::Type SelectType)
{
	if (SelectType != ESelectInfo::Direct)
	{
		SetSelectedItem(Item);
		BP_OnItemSelected.Broadcast(Item);
		OnItemSelected.Broadcast(Item);
	}
}

void UMDViewModelComboBox::UpdateSelectedItemWidget()
{
	if (!ComboBoxContent.IsValid())
	{
		return;
	}

	ComboBoxContent->SetContent(GenerateEntryWidget(SelectedItem));
}

UClass* UMDViewModelComboBox::GetEntryWidgetClass() const
{
	return EntryWidgetClass;
}
