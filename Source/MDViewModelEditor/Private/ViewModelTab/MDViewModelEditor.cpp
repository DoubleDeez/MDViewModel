#include "ViewModelTab/MDViewModelEditor.h"

#include "WidgetBlueprintEditor.h"
#include "ViewModelTab/MDViewModelList.h"
#include "Widgets/Layout/SSplitter.h"


void SMDViewModelEditor::Construct(const FArguments& InArgs, TSharedPtr<FWidgetBlueprintEditor> BlueprintEditor)
{
	ChildSlot
	[
		SNew(SSplitter)
		.Orientation(Orient_Horizontal)
		+SSplitter::Slot()
		.MinSize(350.f)
		.Value(0.15f)
		[
			SNew(SMDViewModelList, BlueprintEditor->GetWidgetBlueprintObj())
		]
		+SSplitter::Slot()
		[
			SNullWidget::NullWidget
		]
	];
}
