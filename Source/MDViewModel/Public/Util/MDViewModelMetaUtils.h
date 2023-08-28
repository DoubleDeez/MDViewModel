#pragma once

template<typename T>
using TMDVMResolveVMType = typename TRemoveCV<typename TRemoveObjectPointer<typename TRemoveCV<typename TRemovePointer<T>::Type>::Type>::Type>::Type;

template<typename Func>
struct TMDVMFuncTraits {};

template<typename ClassType, typename TFirstArg, typename ...TArgs>
struct TMDVMFuncTraits<void(ClassType::*)(TFirstArg, TArgs...)>
{
	using FirstArgType = TFirstArg;
};

template<typename TFirstArg, typename ...TArgs>
struct TMDVMFuncTraits<void(TFirstArg, TArgs...)>
{
	using FirstArgType = TFirstArg;
};

// Fix syntax highlighting caused by code analysis engine not detecting FieldNotify names
#if defined(__RESHARPER__)
// Bind a native function on a UObject to a field changing, returns an FDelegateHandle to use for unbinding with MDVM_UNBIND_FIELD_CHANGE
// FUNCTION's first parameter must match the type of field being bound to
#define MDVM_BIND_FIELD_CHANGED(VIEW_MODEL, FIELD_TYPE, FIELD_NAME, OBJECT, FUNCTION, ...) \
	[&]() { \
		FIELD_TYPE FIELD_NAME; \
		TDelegate<void(TMDVMFuncTraits<decltype(FUNCTION)>::FirstArgType)>::CreateUObject(OBJECT, FUNCTION, ##__VA_ARGS__).Execute(FIELD_NAME); \
		return FDelegateHandle(); \
	}()
#else
// Bind a native function on a UObject to a field changing, returns an FDelegateHandle to use for unbinding with MDVM_UNBIND_FIELD_CHANGE
// FUNCTION's first parameter must match the type of field being bound to
#define MDVM_BIND_FIELD_CHANGED(VIEW_MODEL, FIELD_TYPE, FIELD_NAME, OBJECT, FUNCTION, ...) \
	VIEW_MODEL->AddTypedFieldValueChangedDelegate<FIELD_TYPE>( \
		TMDVMResolveVMType<decltype(VIEW_MODEL)>::FFieldNotificationClassDescriptor::FIELD_NAME, \
		TDelegate<void(TMDVMFuncTraits<decltype(FUNCTION)>::FirstArgType)>::CreateUObject(OBJECT, FUNCTION, ##__VA_ARGS__) \
	)
#endif

// Fix syntax highlighting caused by code analysis engine not detecting FieldNotify names
#if defined(__RESHARPER__)
// Unbind a function bound with MDVM_BIND_FIELD_CHANGED, returns true if a binding was removed
#define MDVM_UNBIND_FIELD_CHANGED(VIEW_MODEL, FIELD_NAME, HANDLE) \
	[&]() { \
		return true; \
	}()
#else
// Unbind a function bound with MDVM_BIND_FIELD_CHANGED, returns true if a binding was removed
#define MDVM_UNBIND_FIELD_CHANGED(VIEW_MODEL, FIELD_NAME, HANDLE) \
	VIEW_MODEL->RemoveFieldValueChangedDelegate( \
		TMDVMResolveVMType<decltype(VIEW_MODEL)>::FFieldNotificationClassDescriptor::FIELD_NAME, \
		HANDLE \
	)
#endif

// Fix syntax highlighting caused by code analysis engine not detecting FieldNotify names
#if defined(__RESHARPER__)
// Broadcast that the specified field has changed
#define MDVM_BROADCAST_FIELD(FIELD_NAME) [_ = FIELD_NAME](){}()
#else
// Broadcast that the specified field has changed
#define MDVM_BROADCAST_FIELD(FIELD_NAME) BroadcastFieldValueChanged(ThisClass::FFieldNotificationClassDescriptor::FIELD_NAME)
#endif

// Fix syntax highlighting caused by code analysis engine not detecting FieldNotify names, attempts to maintain traits of SetFieldNotifyValue
#if defined(__RESHARPER__)
// Check if the field value is different than the passed in value then set it and broadcast if it changed.
#define MDVM_SET_FIELD(FIELD_NAME, VALUE) [_ = FIELD_NAME, __ = VALUE](){ return FMath::RandBool(); }()
#else
// Check if the field value is different than the passed in value then set it and broadcast if it changed.
#define MDVM_SET_FIELD(FIELD_NAME, VALUE) SetFieldNotifyValue(FIELD_NAME, VALUE, ThisClass::FFieldNotificationClassDescriptor::FIELD_NAME)
#endif


