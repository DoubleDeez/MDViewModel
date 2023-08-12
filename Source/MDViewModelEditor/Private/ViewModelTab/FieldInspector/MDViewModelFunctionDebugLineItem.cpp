#include "ViewModelTab/FieldInspector/MDViewModelFunctionDebugLineItem.h"

#include "EdGraphSchema_K2.h"
#include "MDViewModelGraph.h"
#include "ViewModelTab/FieldInspector/DragAndDrop/MDVMDragAndDropWrapperButton.h"
#include "ViewModelTab/FieldInspector/MDViewModelFieldDebugLineItem.h"
#include "WidgetBlueprint.h"
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
			.bCanDrag(bIsCommand)
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
		.bCanDrag(bIsCommand)
		[
			FMDViewModelDebugLineItemBase::GenerateNameWidget(InSearchString)
		];
}

TSharedRef<SWidget> FMDViewModelFunctionDebugLineItem::GenerateValueWidget(TSharedPtr<FString> InSearchString)
{
	return SNew(SMDVMDragAndDropWrapperButton, StaticCastSharedRef<FMDViewModelFunctionDebugLineItem>(AsShared()))
		.bCanDrag(bIsCommand)
		[
			SNew(SWidgetSwitcher)
			.WidgetIndex(this, &FMDViewModelFunctionDebugLineItem::GetShouldDisplayFieldNotifyIndex)
			+SWidgetSwitcher::Slot()
			[
				SNew(SButton)
				.ContentPadding(FMargin(3.0, 2.0))
				.OnClicked(this, &FMDViewModelFunctionDebugLineItem::OnAddOrViewBoundFieldNotifyFunctionClicked)
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
						MakeShared<FMDViewModelFieldDebugLineItem>(ParamProp, nullptr, ParamProp->GetDisplayNameText(), ParamProp->GetToolTipText(), DebugViewModel));
				}
			}
		}
	}

	CachedPropertyItems.GenerateValueArray(CachedChildren.GetValue());
}

FDebugLineItem* FMDViewModelFunctionDebugLineItem::Duplicate() const
{
	return new FMDViewModelFunctionDebugLineItem(FunctionPtr.Get(), DisplayName, Description, DebugViewModel, bIsCommand, bIsFieldNotify, WidgetBP.Get(), ViewModelClass, ViewModelName);
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
		FMDViewModelGraphModule::OnViewModelFieldNotifyRequestedForBlueprint(WidgetBP.Get(), FunctionPtr->GetFName(), ViewModelClass, ViewModelName);
	}

	return FReply::Handled();
}

int32 FMDViewModelFunctionDebugLineItem::GetAddOrViewBoundFieldNotifyFunctionIndex() const
{
	check(bIsFieldNotify);

	return (!FunctionPtr.IsValid() || FMDViewModelGraphModule::DoesBlueprintBindToViewModelFieldNotify(WidgetBP.Get(), FunctionPtr->GetFName(), ViewModelClass, ViewModelName))
		? 0 : 1;
}
