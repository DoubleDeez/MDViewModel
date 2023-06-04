#include "ViewModelTab/MDViewModelTab.h"
#include "WidgetBlueprintEditor.h"
#include "ViewModelTab/MDViewModelEditor.h"

#define LOCTEXT_NAMESPACE "MDViewModelSummoner"

const FName FMDViewModelSummoner::TabID(TEXT("MDViewModelTab"));
const FName FMDViewModelSummoner::DrawerID(TEXT("MDViewModelDrawer"));


FMDViewModelSummoner::FMDViewModelSummoner(TSharedPtr<FWidgetBlueprintEditor> BlueprintEditor)
	: FWorkflowTabFactory(TabID, BlueprintEditor)
	, WeakWidgetBlueprintEditor(BlueprintEditor)
{
	TabLabel = LOCTEXT("ViewModels", "View Models");
	TabIcon = FSlateIcon(FAppStyle::GetAppStyleSetName(),TEXT("FontEditor.Tabs.PageProperties"));

	bIsSingleton = true;

	ViewMenuDescription = LOCTEXT("ViewModels", "View Models");
	ViewMenuTooltip = LOCTEXT("ViewModelsTooltip", "Modify which view models are assigned to this widget");
}

TSharedRef<SWidget> FMDViewModelSummoner::CreateTabBody(const FWorkflowTabSpawnInfo& Info) const
{
	return SNew(SMDViewModelEditor, WeakWidgetBlueprintEditor.Pin());
}
