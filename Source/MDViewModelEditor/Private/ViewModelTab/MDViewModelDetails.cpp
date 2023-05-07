#include "ViewModelTab/MDViewModelDetails.h"

#include "Brushes/SlateColorBrush.h"
#include "ViewModelTab/MDViewModelFieldInspector.h"

void SMDViewModelDetails::Construct(const FArguments& InArgs)
{
	const FSlateBrush* HeaderBrushPtr = &FAppStyle::Get().GetWidgetStyle<FTableViewStyle>("ListView").BackgroundBrush;
	ChildSlot
	.Padding(4.f)
	[
		SNew(SVerticalBox)
		+SVerticalBox::Slot()
		.AutoHeight()
		.HAlign(HAlign_Fill)
		[
			SNew(SBorder)
			.Padding(4.f)
			.BorderImage(HeaderBrushPtr)
			[
				SNew(STextBlock)
				.Text(INVTEXT("View Model Properties"))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
			]
		]
		+SVerticalBox::Slot()
		.FillHeight(1.f)
		.HAlign(HAlign_Fill)
		[
			SAssignNew(FieldInspector, SMDViewModelFieldInspector)
		]
	];

	UpdateViewModel(InArgs._ViewModelClass, InArgs._DebugViewModel);
}

void SMDViewModelDetails::UpdateViewModel(TSubclassOf<UMDViewModelBase> ViewModelClass, UMDViewModelBase* DebugViewModel)
{
	if (FieldInspector.IsValid())
	{
		FieldInspector->SetReferences(ViewModelClass, DebugViewModel);
	}
}
