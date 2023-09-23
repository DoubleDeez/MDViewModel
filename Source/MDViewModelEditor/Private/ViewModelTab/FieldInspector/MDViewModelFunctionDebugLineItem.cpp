#include "ViewModelTab/FieldInspector/MDViewModelFunctionDebugLineItem.h"

#include "EdGraphSchema_K2.h"
#include "HAL/FileManager.h"
#include "Nodes/MDVMNode_ViewModelFieldNotify.h"
#include "Util/MDViewModelGraphStatics.h"
#include "ViewModel/MDViewModelBase.h"
#include "ViewModelTab/FieldInspector/DragAndDrop/MDVMDragAndDropWrapperButton.h"
#include "ViewModelTab/FieldInspector/DragAndDrop/MDVMInspectorDragAndDropCommand.h"
#include "ViewModelTab/FieldInspector/DragAndDrop/MDVMInspectorDragAndDropGetter.h"
#include "ViewModelTab/FieldInspector/MDViewModelFieldDebugLineItem.h"
#include "Widgets/Layout/SWidgetSwitcher.h"

TSharedRef<SWidget> FMDViewModelFunctionDebugLineItem::GetNameIcon()
{
	if (const UFunction* Function = FunctionPtr.Get())
	{
		FLinearColor ReturnValueColor = FLinearColor::White;
		FText ToolTipText = Function->GetToolTipText();
		if (const FProperty* ReturnProperty = Function->GetReturnProperty())
		{
			const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();
			FEdGraphPinType PinType;
			if (K2Schema->ConvertPropertyToPinType(ReturnProperty, PinType))
			{
				ReturnValueColor = K2Schema->GetPinTypeColor(PinType);
			}

			ToolTipText = UEdGraphSchema_K2::TypeToText(ReturnProperty);
		}

		return SNew(SMDVMDragAndDropWrapperButton, StaticCastSharedRef<FMDViewModelFunctionDebugLineItem>(AsShared()))
			.bCanDrag(DragAndDropCreator.IsBound())
			[
				SNew(SImage)
				.Image(FAppStyle::Get().GetBrush(TEXT("GraphEditor.Function_16x")))
				.ColorAndOpacity(ReturnValueColor)
				.ToolTipText(ToolTipText)
			];
	}

	return FDebugLineItem::GetNameIcon();
}

TSharedRef<SWidget> FMDViewModelFunctionDebugLineItem::GenerateNameWidget(TSharedPtr<FString> InSearchString)
{
	return SNew(SMDVMDragAndDropWrapperButton, StaticCastSharedRef<FMDViewModelFunctionDebugLineItem>(AsShared()))
		.bCanDrag(DragAndDropCreator.IsBound())
		[
			FMDViewModelDebugLineItemBase::GenerateNameWidget(InSearchString)
		];
}

TSharedRef<SWidget> FMDViewModelFunctionDebugLineItem::GenerateValueWidget(TSharedPtr<FString> InSearchString)
{
	return SNew(SMDVMDragAndDropWrapperButton, StaticCastSharedRef<FMDViewModelFunctionDebugLineItem>(AsShared()))
		.bCanDrag(DragAndDropCreator.IsBound())
		[
			SNew(SWidgetSwitcher)
			.WidgetIndex(this, &FMDViewModelFunctionDebugLineItem::GetShouldDisplayFieldNotifyIndex)
			+SWidgetSwitcher::Slot()
			[
				SNew(SButton)
				.ContentPadding(FMargin(3.0, 2.0))
				.OnClicked(this, &FMDViewModelFunctionDebugLineItem::OnAddOrViewBoundFieldNotifyFunctionClicked)
				.IsEnabled(this, &FMDViewModelFunctionDebugLineItem::CanCreateNodes)
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
				SNew(STextBlock)
				.Text(this, &FMDViewModelFunctionDebugLineItem::GetDescription)
				.ToolTipText(this, &FMDViewModelFunctionDebugLineItem::GetDescription)
			]
		];
}

TSharedRef<FMDVMInspectorDragAndDropActionBase> FMDViewModelFunctionDebugLineItem::CreateDragAndDropAction() const
{
	check(DragAndDropCreator.IsBound());

	return DragAndDropCreator.Execute(GetFunction(), GetViewModelAssignmentReference());
}

void FMDViewModelFunctionDebugLineItem::UpdateIsDebugging(bool InIsDebugging)
{
	bIsDebugging = InIsDebugging;
}

void FMDViewModelFunctionDebugLineItem::UpdateCachedChildren() const
{
	CachedChildren = TArray<FDebugTreeItemPtr>();

	if (const UFunction* Function = FunctionPtr.Get())
	{
		for (TFieldIterator<const FProperty> It(Function); It; ++It)
		{
			if (const FProperty* ParamProp = *It)
			{
				if (!CachedPropertyItems.Contains(ParamProp->GetFName()))
				{
					CachedPropertyItems.Add(ParamProp->GetFName(),
						MakeShared<FMDViewModelFieldDebugLineItem>(ParamProp, nullptr, ParamProp->GetDisplayNameText(), ParamProp->GetToolTipText(), DebugViewModel, BlueprintEditorPtr));
				}
			}
		}
	}

	CachedPropertyItems.GenerateValueArray(CachedChildren.GetValue());
}

FDebugLineItem* FMDViewModelFunctionDebugLineItem::Duplicate() const
{
	return new FMDViewModelFunctionDebugLineItem(FunctionPtr.Get(), DisplayName, Description, DebugViewModel, BlueprintEditorPtr, DragAndDropCreator, bIsFieldNotify, ViewModelClass, ViewModelName);
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
		const UMDVMNode_ViewModelFieldNotify* Node = FMDViewModelGraphStatics::FindExistingViewModelFieldNotifyNode(BlueprintPtr.Get(), FunctionPtr->GetFName(), { ViewModelClass, ViewModelName });
		if (IsValid(Node))
		{
			Result += Node->GetFindReferenceSearchString();
		}
	}

	if (FunctionPtr.IsValid() && IsValid(ViewModelClass))
	{
		const FString FunctionName = FunctionPtr->GetDisplayNameText().ToString();
		const FString ViewModelClassName = ViewModelClass->GetDisplayNameText().ToString();
		FString FunctionString = TEXT("(\"") + FunctionName + TEXT("\" && (")
		+ FString::Printf(TEXT("\"%s - %s\""), *ViewModelClassName, *ViewModelName.ToString()) += TEXT(" || ")
		+ FString::Printf(TEXT("\"%s (%s)\""), *ViewModelClassName, *ViewModelName.ToString()) += TEXT("))");

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

int32 FMDViewModelFunctionDebugLineItem::GetShouldDisplayFieldNotifyIndex() const
{
	return (!bIsDebugging && bIsFieldNotify) ? 0 : 1;
}

FReply FMDViewModelFunctionDebugLineItem::OnAddOrViewBoundFieldNotifyFunctionClicked() const
{
	check(bIsFieldNotify);

	if (FunctionPtr.IsValid())
	{
		FMDViewModelGraphStatics::OnViewModelFieldNotifyRequestedForBlueprint(BlueprintPtr.Get(), FunctionPtr->GetFName(), { ViewModelClass, ViewModelName });
	}

	return FReply::Handled();
}

int32 FMDViewModelFunctionDebugLineItem::GetAddOrViewBoundFieldNotifyFunctionIndex() const
{
	check(bIsFieldNotify);

	return (!FunctionPtr.IsValid() || FMDViewModelGraphStatics::DoesBlueprintBindToViewModelFieldNotify(BlueprintPtr.Get(), FunctionPtr->GetFName(), { ViewModelClass, ViewModelName }))
		? 0 : 1;
}
