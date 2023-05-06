// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class FWidgetBlueprintEditor;

/**
 *
 */
class MDVIEWMODELEDITOR_API SMDViewModelEditor : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SMDViewModelEditor)
		{
		}

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, TSharedPtr<FWidgetBlueprintEditor> BlueprintEditor);
};
