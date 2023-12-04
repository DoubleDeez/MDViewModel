#include "ViewModelTab/FieldInspector/MDViewModelFunctionDebugLineItem.h"

#include "EdGraphSchema_K2.h"
#include "HAL/FileManager.h"
#include "MDViewModelEditorConfig.h"
#include "Nodes/MDVMNode_ViewModelFieldNotify.h"
#include "Util/MDViewModelGraphStatics.h"
#include "ViewModel/MDViewModelBase.h"
#include "ViewModelTab/FieldInspector/DragAndDrop/MDVMDragAndDropWrapperButton.h"
#include "ViewModelTab/FieldInspector/DragAndDrop/MDVMInspectorDragAndDropFieldNotify.h"
#include "ViewModelTab/FieldInspector/MDViewModelFieldDebugLineItem.h"
#include "Widgets/Layout/SWidgetSwitcher.h"

FMDViewModelFunctionDebugLineItem::FMDViewModelFunctionDebugLineItem(const TWeakPtr<FBlueprintEditor>& BlueprintEditorPtr, const FMDViewModelAssignmentReference& Assignment, const UFunction* Function, const FMDVMDragAndDropCreatorFunc& DragAndDropCreator, bool bIsFieldNotify)
	: FMDViewModelDebugLineItemBase(BlueprintEditorPtr, Assignment)
	, FunctionPtr(Function)
	, DragAndDropCreator(DragAndDropCreator)
	, bIsFieldNotify(bIsFieldNotify)
{
}

FMDViewModelFunctionDebugLineItem::~FMDViewModelFunctionDebugLineItem()
{
	CleanUpGetterReturnValue();
}

TSharedRef<SWidget> FMDViewModelFunctionDebugLineItem::GetNameIcon()
{
	if (const UFunction* Function = FunctionPtr.Get())
	{
		FLinearColor ReturnValueColor = FLinearColor::White;
		FText ToolTipText = Function->GetToolTipText();
		if (const FProperty* ReturnProperty = MDViewModelUtils::GetFunctionReturnProperty(Function))
		{
			const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();
			FEdGraphPinType PinType;
			if (K2Schema->ConvertPropertyToPinType(ReturnProperty, PinType))
			{
				ReturnValueColor = K2Schema->GetPinTypeColor(PinType);
			}

			ToolTipText = UEdGraphSchema_K2::TypeToText(ReturnProperty);
		}

		return SNew(SMDVMDragAndDropWrapperButton)
			.OnGetDragAndDropAction(this, &FMDViewModelFunctionDebugLineItem::CreateDragAndDropAction)
			.bCanDrag(this, &FMDViewModelFunctionDebugLineItem::CanDrag)
			[
				SNew(SImage)
				.Image(FAppStyle::Get().GetBrush(TEXT("GraphEditor.Function_16x")))
				.ColorAndOpacity(ReturnValueColor)
				.ToolTipText(ToolTipText)
			];
	}

	return FDebugLineItem::GetNameIcon();
}

TSharedRef<SWidget> FMDViewModelFunctionDebugLineItem::GenerateValueWidget(TSharedPtr<FString> InSearchString)
{
	return SNew(SWidgetSwitcher)
		.WidgetIndex(this, &FMDViewModelFunctionDebugLineItem::GetShouldDisplayFieldNotifyIndex)
		+SWidgetSwitcher::Slot()
		[
			SNew(SMDVMDragAndDropWrapperButton)
			.bCanDrag(this, &FMDViewModelFunctionDebugLineItem::CanAddBoundFieldNotifyFunction)
			.OnGetDragAndDropAction(this, &FMDViewModelFunctionDebugLineItem::CreateBindingDragAndDropAction)
			.ButtonArguments(
				SButton::FArguments()
				.ContentPadding(FMargin(3.0, 2.0))
				.OnClicked(this, &FMDViewModelFunctionDebugLineItem::OnAddOrViewBoundFieldNotifyFunctionClicked)
				.IsEnabled(this, &FMDViewModelFunctionDebugLineItem::CanCreateNodes)
			)
			[
				SNew(SWidgetSwitcher)
				.WidgetIndex(this, &FMDViewModelFunctionDebugLineItem::GetAddOrViewBoundFieldNotifyFunctionIndex)
				+SWidgetSwitcher::Slot()
				[
					SNew(SImage)
					.ColorAndOpacity(FSlateColor::UseForeground())
					.Image(FAppStyle::Get().GetBrush("Icons.SelectInViewport"))
					.ToolTipText(INVTEXT("Focus the existing bound function."))
				]
				+SWidgetSwitcher::Slot()
				[
					SNew(SImage)
					.ColorAndOpacity(FSlateColor::UseForeground())
					.Image(FAppStyle::Get().GetBrush("Icons.Plus"))
					.ToolTipText(INVTEXT("Create a BP event bound to this view model field notify function"))
				]
			]
		]
		+SWidgetSwitcher::Slot()
		[
			SNew(SMDVMDragAndDropWrapperButton)
			.bCanDrag(this, &FMDViewModelFunctionDebugLineItem::CanDrag)
			.OnGetDragAndDropAction(this, &FMDViewModelFunctionDebugLineItem::CreateDragAndDropAction)
			[
				SNew(STextBlock)
				.Text(this, &FMDViewModelFunctionDebugLineItem::GetDisplayValue)
				.ToolTipText(this, &FMDViewModelFunctionDebugLineItem::GetDescription)
			]
		];
}

TSharedRef<FMDVMInspectorDragAndDropActionBase> FMDViewModelFunctionDebugLineItem::CreateDragAndDropAction() const
{
	check(DragAndDropCreator.IsBound());

	return DragAndDropCreator.Execute(GetFunction(), GetViewModelAssignmentReference());
}

TSharedRef<FMDVMInspectorDragAndDropActionBase> FMDViewModelFunctionDebugLineItem::CreateBindingDragAndDropAction() const
{
	return FMDVMInspectorDragAndDropFieldNotify::Create(
		FunctionPtr.Get(),
		GetViewModelAssignmentReference()
	);
}

void FMDViewModelFunctionDebugLineItem::UpdateCachedChildren() const
{
	TryUpdateGetterReturnValue();

	CachedChildren = TArray<FDebugTreeItemPtr>();

	if (const UFunction* Function = FunctionPtr.Get())
	{
		for (TFieldIterator<const FProperty> It(Function); It; ++It)
		{
			if (const FProperty* ParamProp = *It)
			{
				if (!CachedPropertyItems.Contains(ParamProp->GetFName()))
				{
					void* ValuePtr = (ParamProp == MDViewModelUtils::GetFunctionReturnProperty(Function)) ? GetterReturnValuePtr : nullptr;
					TSharedPtr<FMDViewModelFieldDebugLineItem> Item = MakeShared<FMDViewModelFieldDebugLineItem>(BlueprintEditorPtr, Assignment, ParamProp, ValuePtr);
					Item->SetDisplayText(ParamProp->GetDisplayNameText(), ParamProp->GetToolTipText());
					Item->UpdateDebugging(bIsDebugging, DebugViewModel);
					CachedPropertyItems.Add(ParamProp->GetFName(), Item);
				}
				else
				{
					TSharedPtr<FMDViewModelFieldDebugLineItem> Item = StaticCastSharedPtr<FMDViewModelFieldDebugLineItem>(CachedPropertyItems[ParamProp->GetFName()]);
					Item->UpdateDebugging(bIsDebugging, DebugViewModel);
					Item->UpdateViewModel(Assignment);

					if (Item->GetValuePtr() != GetterReturnValuePtr && ParamProp == MDViewModelUtils::GetFunctionReturnProperty(Function))
					{
						Item->UpdateValuePtr(GetterReturnValuePtr);
					}
				}
			}
		}
	}

	CachedPropertyItems.GenerateValueArray(CachedChildren.GetValue());
}

FDebugLineItem* FMDViewModelFunctionDebugLineItem::Duplicate() const
{
	FMDViewModelFunctionDebugLineItem* Item = new FMDViewModelFunctionDebugLineItem(BlueprintEditorPtr, Assignment, FunctionPtr.Get(), DragAndDropCreator, bIsFieldNotify);
	Item->SetDisplayText(DisplayName, Description);
	Item->UpdateDebugging(bIsDebugging, DebugViewModel);

	return Item;
}

bool FMDViewModelFunctionDebugLineItem::CanCreateNodes() const
{
	return FMDViewModelDebugLineItemBase::CanCreateNodes() || GetAddOrViewBoundFieldNotifyFunctionIndex() == 0;
}

FString FMDViewModelFunctionDebugLineItem::GenerateSearchString() const
{
	FString Result;

	if (bIsFieldNotify && FunctionPtr.IsValid())
	{
		const UMDVMNode_ViewModelFieldNotify* Node = FMDViewModelGraphStatics::FindExistingViewModelFieldNotifyNode(BlueprintPtr.Get(), FunctionPtr->GetFName(), Assignment);
		if (IsValid(Node))
		{
			Result += Node->GetFindReferenceSearchString();
		}
	}

	if (FunctionPtr.IsValid() && Assignment.IsAssignmentValid())
	{
		const FString FunctionName = FunctionPtr->GetDisplayNameText().ToString();
		const FString ViewModelClassName = Assignment.ViewModelClass.LoadSynchronous()->GetDisplayNameText().ToString();
		FString FunctionString = TEXT("(\"") + FunctionName + TEXT("\" && (")
		+ FString::Printf(TEXT("\"%s - %s\""), *ViewModelClassName, *Assignment.ViewModelName.ToString()) += TEXT(" || ")
		+ FString::Printf(TEXT("\"%s (%s)\""), *ViewModelClassName, *Assignment.ViewModelName.ToString()) += TEXT("))");

		if (Result.IsEmpty())
		{
			Result = FunctionString;
		}
		else
		{
			Result += TEXT(" || ") + FunctionString;
		}
	}

	return Result;
}

FFieldVariant FMDViewModelFunctionDebugLineItem::GetFieldForDefinitionNavigation() const
{
	return FFieldVariant(FunctionPtr.Get());
}

FText FMDViewModelFunctionDebugLineItem::GetDisplayValue() const
{
	TryUpdateGetterReturnValue();

	const UFunction* Function = GetFunction();
	const FProperty* Property = IsValid(Function) ? MDViewModelUtils::GetFunctionReturnProperty(Function) : nullptr;

	if (Property != nullptr && GetterReturnValuePtr != nullptr)
	{
		return GeneratePropertyDisplayValue(Property, GetterReturnValuePtr);
	}
	else
	{
		return GetDescription();
	}
}

bool FMDViewModelFunctionDebugLineItem::CanDisplayReturnValue() const
{
	// Only const or pure functions that take no params can be called to get their return value
	// The goal is to detect "Property Getters" to show the value of

	const UFunction* Function = GetFunction();
	if (Function == nullptr)
	{
		return false;
	}

	if (MDViewModelUtils::GetFunctionReturnProperty(Function) == nullptr || Function->NumParms != 1)
	{
		return false;
	}

	if (!Function->HasAnyFunctionFlags(FUNC_Const | FUNC_BlueprintPure))
	{
		return false;
	}

	return true;
}

void FMDViewModelFunctionDebugLineItem::OnDebuggingChanged()
{
	TryUpdateGetterReturnValue();
}

bool FMDViewModelFunctionDebugLineItem::CanDrag() const
{
	return DragAndDropCreator.IsBound();
}

int32 FMDViewModelFunctionDebugLineItem::GetShouldDisplayFieldNotifyIndex() const
{
	const bool bShowFieldNotify = bIsFieldNotify && !bIsDebugging;
	return bShowFieldNotify ? 0 : 1;
}

FReply FMDViewModelFunctionDebugLineItem::OnAddOrViewBoundFieldNotifyFunctionClicked() const
{
	check(bIsFieldNotify);

	if (FunctionPtr.IsValid())
	{
		FMDViewModelGraphStatics::OnViewModelFieldNotifyRequestedForBlueprint(BlueprintPtr.Get(), FunctionPtr->GetFName(), Assignment);
	}

	return FReply::Handled();
}

bool FMDViewModelFunctionDebugLineItem::CanAddBoundFieldNotifyFunction() const
{
	return (FunctionPtr.IsValid() && !FMDViewModelGraphStatics::DoesBlueprintBindToViewModelFieldNotify(BlueprintPtr.Get(), FunctionPtr->GetFName(), Assignment));
}

int32 FMDViewModelFunctionDebugLineItem::GetAddOrViewBoundFieldNotifyFunctionIndex() const
{
	check(bIsFieldNotify);

	return CanAddBoundFieldNotifyFunction()	? 1 : 0;
}

void FMDViewModelFunctionDebugLineItem::TryUpdateGetterReturnValue() const
{
	if (!bIsDebugging || !GetDefault<UMDViewModelEditorConfig>()->bEnableReturnValuePreviewing || !CanDisplayReturnValue())
	{
		CleanUpGetterReturnValue();
		return;
	}

	UFunction* Function = const_cast<UFunction*>(GetFunction());
	if (Function == nullptr)
	{
		CleanUpGetterReturnValue();
		return;
	}

	UMDViewModelBase* ViewModel = DebugViewModel.Get();
	if (!IsValid(ViewModel))
	{
		CleanUpGetterReturnValue();
		return;
	}

	const FProperty* ReturnProp = MDViewModelUtils::GetFunctionReturnProperty(Function);
	if (ReturnProp == nullptr)
	{
		CleanUpGetterReturnValue();
		return;
	}

	if (GetterReturnValuePtr == nullptr)
	{
		GetterReturnValuePtr = FMemory::Malloc(ReturnProp->GetSize(), ReturnProp->GetMinAlignment());
		ReturnProp->InitializeValue(GetterReturnValuePtr);
	}

	ViewModel->ProcessEvent(Function, GetterReturnValuePtr);

	for (const TPair<FName, FDebugTreeItemPtr>& Pair : CachedPropertyItems)
	{
		const TSharedPtr<FMDViewModelFieldDebugLineItem> Item = StaticCastSharedPtr<FMDViewModelFieldDebugLineItem>(Pair.Value);
		if (Item->GetProperty() == ReturnProp)
		{
			Item->UpdateValuePtr(GetterReturnValuePtr);
		}
	}

	CachedChildren.Reset();
}

void FMDViewModelFunctionDebugLineItem::CleanUpGetterReturnValue() const
{
	if (GetterReturnValuePtr != nullptr)
	{
		for (const auto& Pair : CachedPropertyItems)
		{
			const TSharedPtr<FMDViewModelFieldDebugLineItem> ChildItem = StaticCastSharedPtr<FMDViewModelFieldDebugLineItem>(Pair.Value);
			ChildItem->UpdateValuePtr(nullptr);
		}

		CachedChildren.Reset();

		FMemory::Free(GetterReturnValuePtr);
		GetterReturnValuePtr = nullptr;
	}
}
