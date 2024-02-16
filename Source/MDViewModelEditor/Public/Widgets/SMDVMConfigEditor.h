#pragma once

#include "Misc/NotifyHook.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SCompoundWidget.h"

class IDetailsView;

class MDVIEWMODELEDITOR_API SMDVMConfigEditor : public SCompoundWidget, public FNotifyHook
{
public:
	SLATE_BEGIN_ARGS(SMDVMConfigEditor)
		{
		}

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	
	virtual void NotifyPostChange(const FPropertyChangedEvent& PropertyChangedEvent, FProperty* PropertyThatChanged) override;
	virtual int32 OnPaint( const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled ) const override;

private:
	void OnClassPicked(UClass* Class);
	
	TSharedPtr<IDetailsView> ViewModelDetails;
	SVerticalBox::FSlot* ClassViewerSlot = nullptr;
};
