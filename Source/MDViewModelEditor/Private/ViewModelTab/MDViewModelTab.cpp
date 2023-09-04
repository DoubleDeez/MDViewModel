#include "ViewModelTab/MDViewModelTab.h"

#include "Framework/Application/SlateApplication.h"
#include "ViewModelTab/MDViewModelEditor.h"
#include "Widgets/Docking/SDockTab.h"
#include "WidgetBlueprintEditor.h"

#define LOCTEXT_NAMESPACE "MDViewModelSummoner"

const FName FMDViewModelSummoner::TabID(TEXT("MDViewModelTab"));
const FName FMDViewModelSummoner::DrawerID(TEXT("MDViewModelDrawer"));


FMDViewModelSummoner::FMDViewModelSummoner(const TSharedPtr<FBlueprintEditor>& BlueprintEditor, bool bIsDrawer)
	: FWorkflowTabFactory(TabID, BlueprintEditor)
	, WeakBlueprintEditor(BlueprintEditor)
	, bIsDrawer(bIsDrawer)
{
	TabLabel = LOCTEXT("ViewModels", "View Models");
	TabIcon = FSlateIcon(FAppStyle::GetAppStyleSetName(),TEXT("FontEditor.Tabs.PageProperties"));

	bIsSingleton = true;

	ViewMenuDescription = LOCTEXT("ViewModels", "View Models");
	ViewMenuTooltip = LOCTEXT("ViewModelsTooltip", "Modify which view models are assigned to this widget");
}

TSharedRef<SWidget> FMDViewModelSummoner::CreateTabBody(const FWorkflowTabSpawnInfo& Info) const
{
	return GetOrCreateViewModelEditor();
}

TSharedRef<SWidget> FMDViewModelSummoner::GetOrCreateViewModelEditor() const
{
	if (!ViewModelEditor.IsValid())
	{
		// For Drawers on Widget BPs, use the same VM Editor widget as we switch modes so state is maintained
		static const FName WidgetBlueprintEditorName = TEXT("WidgetBlueprintEditor");
		TSharedPtr<FBlueprintEditor> BlueprintEditor = WeakBlueprintEditor.Pin();
		if (bIsDrawer && BlueprintEditor.IsValid() && BlueprintEditor->GetEditorName() == WidgetBlueprintEditorName)
		{
			const TSharedPtr<FWidgetBlueprintEditor> WidgetBlueprintEditor = StaticCastSharedPtr<FWidgetBlueprintEditor>(BlueprintEditor);
			ViewModelEditor = StaticCastSharedPtr<SMDViewModelEditor>(WidgetBlueprintEditor->GetExternalEditorWidget(DrawerID));
			if (!ViewModelEditor.IsValid())
			{
				SAssignNew(ViewModelEditor, SMDViewModelEditor, BlueprintEditor);
				WidgetBlueprintEditor->AddExternalEditorWidget(DrawerID, ViewModelEditor.ToSharedRef());
			}
		}
		else
		{
			SAssignNew(ViewModelEditor, SMDViewModelEditor, BlueprintEditor);
		}
	}

	return ViewModelEditor.ToSharedRef();
}

TSharedRef<SDockTab> FMDViewModelSummoner::StaticSpawnTab(const FSpawnTabArgs& Args, TWeakPtr<FBlueprintEditor> WeakBlueprintEditor)
{
	if (const TSharedPtr<FBlueprintEditor> BlueprintEditor = WeakBlueprintEditor.Pin())
	{
		constexpr bool IsDrawer = false;
		return FMDViewModelSummoner(WeakBlueprintEditor.Pin(), IsDrawer).OnSpawnTab(Args, BlueprintEditor->GetTabManager());
	}

	return SNew(SDockTab);
}

FWidgetDrawerConfig FMDViewModelSummoner::CreateDrawerConfig(const TSharedRef<FBlueprintEditor>& BlueprintEditor)
{
	// The summoner is kept alive by being captured in the drawer config lambdas.
	constexpr bool IsDrawer = true;
	TSharedRef<FMDViewModelSummoner> Summoner = MakeShared<FMDViewModelSummoner>(BlueprintEditor, IsDrawer);

	FWidgetDrawerConfig DrawerConfig(DrawerID);
	DrawerConfig.GetDrawerContentDelegate.BindSP(Summoner, &FMDViewModelSummoner::GetOrCreateViewModelEditor);
	DrawerConfig.OnDrawerOpenedDelegate.BindLambda([Summoner](FName StatusBarWithDrawerName)
	{
		FSlateApplication::Get().SetUserFocus(FSlateApplication::Get().GetUserIndexForKeyboard(), Summoner->GetOrCreateViewModelEditor());
	});
	DrawerConfig.OnDrawerDismissedDelegate.BindLambda([Summoner](const TSharedPtr<SWidget>& NewlyFocusedWidget)
	{
		if (const TSharedPtr<FBlueprintEditor> BlueprintEditor = Summoner->WeakBlueprintEditor.Pin())
		{
			BlueprintEditor->SetKeyboardFocus();
		}
	});
	DrawerConfig.ButtonText = Summoner->TabLabel;
	DrawerConfig.ToolTipText = Summoner->ViewMenuTooltip;
	DrawerConfig.Icon = Summoner->TabIcon.GetIcon();

	return DrawerConfig;
}
