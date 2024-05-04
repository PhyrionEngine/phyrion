#pragma once

#include "Fyrion/Common.hpp"
#include "Allocator.hpp"
#include "StringView.hpp"
#include "TypeInfo.hpp"
#include "String.hpp"
#include "SharedPtr.hpp"
#include "HashMap.hpp"
#include "Span.hpp"

namespace Fyrion
{

    class TypeBuilder;
    class FunctionBuilder;
    class ConstructorBuilder;
    class FieldBuilder;
    class AttributeBuilder;
    class TypeHandler;
    struct FieldInfo;

    typedef VoidPtr (*FnCast)(const TypeHandler* typeHandler, VoidPtr derived);

    struct FunctionInfo
    {
        TypeID functionId{};
        TypeID ownerId{};
        usize paramCount{};
        const FieldInfo* paramsInfo{};
        const FieldInfo* returnInfo{};
    };

    struct FieldInfo
    {
        TypeID ownerId{};
        bool isConst{};
        bool isPointer{};
        bool isReference{};
        TypeInfo typeInfo{};
    };

    template<typename Owner, typename Field>
    constexpr FieldInfo MakeFieldInfo()
    {
        return FieldInfo{
            .ownerId = GetTypeID<Owner>(),
            .isConst = false,
            .isPointer  = false,
            .isReference  = false,
            .typeInfo = GetTypeInfo<Field>()
        };
    }

    struct FunctionHandlerCreation
    {
        StringView            name{};
        TypeID                functionId{};
        TypeID                owner{};
        Span<const FieldInfo> params{};
        FieldInfo             retInfo{};
    };

    struct AttributeInfo
    {
        typedef ConstPtr(*FnGetValue)(const AttributeInfo* handler);
        typedef TypeInfo(*FnGetTypeInfo)(const AttributeInfo* handler);

        VoidPtr         userData{};

        FnGetValue      GetValue{};
        FnGetTypeInfo   GetInfo{};
    };

    class FY_API AttributeHandler
    {
    public:
        ConstPtr                GetAttribute(TypeID attributeId) const;
        bool                    HasAttribute(TypeID attributeId) const;
        Span<AttributeInfo*>    GetAttributes() const;

        template<typename AttType>
        const AttType* GetAttribute() const
        {
            return static_cast<const AttType*>(GetAttribute(GetTypeID<AttType>()));
        }

        template<typename AttType>
        bool HasAttribute() const
        {
            return HasAttribute(GetTypeID<AttType>());
        }


        friend class AttributeBuilder;
    protected:
        virtual void OnAttributeCreated(TypeID attributeId) {};
    private:
        HashMap<TypeID, SharedPtr<AttributeInfo>>  m_attributes{};
        Array<AttributeInfo*>                      m_attributeArray{};
    };

    class FY_API ParamHandler
    {
    public:
        ParamHandler(usize index, const FieldInfo& fieldInfo);
        const FieldInfo&    GetFieldInfo() const;
        const String&       GetName() const;
    private:
        FieldInfo   m_fieldInfo{};
        String      m_name{};
    };


    class FY_API ConstructorHandler : public AttributeHandler
    {
    public:
        typedef void (*PlacementNewFn)(ConstructorHandler* handler, VoidPtr memory, VoidPtr* params);
        typedef VoidPtr (*NewInstanceFn)(ConstructorHandler* handler, Allocator& allocator, VoidPtr* params);

        ConstructorHandler(FieldInfo* params,usize paramsCount);

        VoidPtr     NewInstance(Allocator& allocator, VoidPtr* params);
        void        Construct(VoidPtr memory, VoidPtr* params);

        friend class ConstructorBuilder;
    private:
        PlacementNewFn      m_placementNewFn;
        NewInstanceFn       m_newInstanceFn;
        Array<ParamHandler> m_params{};
    };

    class FY_API FieldHandler : public AttributeHandler
    {
    public:
        typedef FieldInfo   (*FnGetFieldInfo)(const FieldHandler* fieldHandler);
        typedef VoidPtr     (*FnGetFieldPointer)(const FieldHandler* fieldHandler, VoidPtr instance);
        typedef void        (*FnCopyValueTo)(const FieldHandler* fieldHandler, ConstPtr instance, VoidPtr value);
        typedef void        (*FnSetValue)(const FieldHandler* fieldHandler, VoidPtr instance, ConstPtr value);
        typedef void        (*FnGetValue)(const FieldHandler* fieldHandler, VoidPtr instance, VoidPtr result);

        explicit FieldHandler(const String& name);

        StringView  GetName() const;
        FieldInfo   GetFieldInfo() const;
        VoidPtr     GetFieldPointer(VoidPtr instance) const;
        void        CopyValueTo(ConstPtr instance, VoidPtr value);
        void        SetValue(VoidPtr instance, ConstPtr value);

        template<typename T>
        T& GetValueAs(VoidPtr instance)
        {
            return *static_cast<T*>(GetFieldPointer(instance));
        }

        template<typename T>
        void SetValueAs(VoidPtr instance, const T& value)
        {
            SetValue(instance, &value);
        }

        friend class FieldBuilder;
    private:
        String            m_name;
        FnGetFieldInfo    m_fnGetFieldInfo;
        FnGetFieldPointer m_fnGetFieldPointer;
        FnCopyValueTo     m_fnCopyValueTo;
        FnSetValue        m_fnSetValue;
        FnGetValue        m_fnGetValue;
    };

    class FY_API FunctionHandler : public AttributeHandler
    {
        friend class FunctionBuilder;
    public:
        typedef void(*FnInvoke)(const FunctionHandler* handler, VoidPtr instance, VoidPtr ret, VoidPtr* params);
    private:

        String              m_name{};
        String              m_simpleName{};
        TypeID              m_functionId{U64_MAX};
        TypeID              m_owner{};
        Array<ParamHandler> m_params{};
        FieldInfo           m_return{};
        FnInvoke            m_fnInvoke{};
        VoidPtr             m_functionPointer{};

        HashMap<TypeID, SharedPtr<AttributeHandler>> m_Attributes;
        Array<AttributeHandler*>                     m_AttributeArray;
    public:

        StringView          GetName() const;
        Span<ParamHandler>  GetParams() const;
        FieldInfo           GetReturn() const;
        TypeID              GetOwner() const;
        VoidPtr             GetFunctionPointer() const;

        void Invoke(VoidPtr instance, VoidPtr ret, VoidPtr* params) const;

    protected:
        void OnAttributeCreated(TypeID attributeId) override;
    };

    struct DerivedType
    {

        TypeID typeId{};
        FnCast fnCast{};
    };


    class FY_API TypeHandler : public AttributeHandler
    {
    public:
        typedef void (*FnDestroy)(const TypeHandler* typeHandler, Allocator& allocator, VoidPtr instance);
        typedef void (*FnDestructor)(const TypeHandler* typeHandler, VoidPtr instance);
        typedef void (*FnCopy)(const TypeHandler* typeHandler, ConstPtr source, VoidPtr dest);
        typedef void (*FnMove)(const TypeHandler* typeHandler, VoidPtr source, VoidPtr dest);

        friend class TypeBuilder;
    private:

        String       m_name{};
        String       m_simpleName{};
        TypeInfo     m_typeInfo{};
        u32          m_version{};
        FnDestroy    m_fnDestroy{};
        FnCopy       m_fnCopy{};
        FnDestructor m_fnDestructor{};
        FnMove       m_fnMove{};

        HashMap<usize, SharedPtr<ConstructorHandler>> m_constructors{};
        Array<ConstructorHandler*>                    m_constructorArray{};
        HashMap<String, SharedPtr<FieldHandler>>      m_fields{};
        Array<FieldHandler*>                          m_fieldArray{};
        HashMap<String, SharedPtr<FunctionHandler>>   m_functions{};
        Array<FunctionHandler*>                       m_functionArray{};

        HashMap<TypeID, FnCast>                       m_baseTypes{};
        Array<TypeID>                                 m_baseTypesArray{};
        Array<DerivedType>                            m_derivedTypes{};
    public:
        TypeHandler(const StringView& name, const TypeInfo& typeInfo, u32 version);

        ConstructorHandler*             FindConstructor(TypeID* ids, usize size) const;
        Span<ConstructorHandler*>       GetConstructors() const;

        FieldHandler*                   FindField(const StringView& fieldName) const;
        Span<FieldHandler*>             GetFields() const;

        FunctionHandler*                FindFunction(const StringView& functionName) const;
        Span<FunctionHandler*>          GetFunctions() const;

        Span<DerivedType>               GetDerivedTypes() const;
        Array<TypeID>                   GetBaseTypes() const;


        StringView          GetName() const;
        StringView          GetSimpleName() const;
        const TypeInfo&     GetTypeInfo() const;
        u32                 GetVersion() const;

        void                Destroy(VoidPtr instance, Allocator& allocator = MemoryGlobals::GetDefaultAllocator()) const;
        void                Destructor(VoidPtr instance) const;
        void                Copy(ConstPtr source, VoidPtr dest) const;
        void                Move(VoidPtr source, VoidPtr dest) const;
        VoidPtr             Cast(TypeID  typeId, VoidPtr instance) const;

        VoidPtr NewInstance(Allocator& allocator = MemoryGlobals::GetDefaultAllocator()) const
        {
            if (ConstructorHandler* constructor = FindConstructor(nullptr, 0))
            {
                return constructor->NewInstance(allocator, nullptr);
            }
            return nullptr;
        }

        template<typename ...Args>
        VoidPtr NewInstance(Allocator& allocator, Args&& ...args) const
        {
            TypeID ids[] = {GetTypeID<Args>()...,};

            if (ConstructorHandler* constructor = FindConstructor(ids, sizeof...(args)))
            {
                VoidPtr params[] = {&args...};
                return constructor->NewInstance(allocator, params);
            }
            return nullptr;
        }

        template<typename ...Args>
        VoidPtr NewInstance(Args&& ...args) const
        {
            return NewInstance(MemoryGlobals::GetDefaultAllocator(), Traits::Forward<Args>(args)...);
        }

        void Construct(VoidPtr memory)
        {
            if (ConstructorHandler* constructor = FindConstructor(nullptr, 0))
            {
                return constructor->Construct(memory, nullptr);
            }
        }

        template<typename T>
        T* Cast(VoidPtr instance) const
        {
          return static_cast<T*>(Cast(GetTypeID<T>(), instance));
        }
    };

    ///-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    ///-------------------------------------------------------------------------------------Builders----------------------------------------------------------------------------------------------------
    ///-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

    class FY_API AttributeBuilder
    {
    private:
        AttributeHandler& m_attributeHandler;
    public:
        explicit AttributeBuilder(AttributeHandler& attributeHandler);
        AttributeInfo& NewAttribute(TypeID attributeId);
    };


    class FY_API ConstructorBuilder
    {
    private:
        ConstructorHandler& m_constructorHandler;
    public:
        ConstructorBuilder(ConstructorHandler& constructorHandler);

        void SetPlacementNewFn(ConstructorHandler::PlacementNewFn placementNew);
        void SetNewInstanceFn(ConstructorHandler::NewInstanceFn newInstance);
    };

    class FY_API FieldBuilder
    {
    private:
        FieldHandler& m_fieldHandler;
    public:
        explicit FieldBuilder(FieldHandler& fieldHandler);

        void SetFnGetFieldInfo(FieldHandler::FnGetFieldInfo fnGetFieldInfo);
        void SetFnGetFieldPointer(FieldHandler::FnGetFieldPointer fnGetFieldPointer);
        void SetFnCopyValueTo(FieldHandler::FnCopyValueTo fnCopyValueTo);
        void SetFnSetValue(FieldHandler::FnSetValue fnSetValue);
        void SetFnGetValue(FieldHandler::FnGetValue fnGetValue);
    };

    class FY_API FunctionBuilder
    {
    private:
        FunctionHandler& m_functionHandler;
    public:
        explicit    FunctionBuilder(FunctionHandler& functionHandler);
        void        Create(const FunctionHandlerCreation& creation);

        FunctionHandler& GetFunctionHandler() const;

        void SetFnInvoke(FunctionHandler::FnInvoke fnInvoke);
        void SetFunctionPointer(VoidPtr functionPointer);
    };

    class FY_API TypeBuilder
    {
    public:
        TypeBuilder(TypeHandler& typeHandler);

        void                    SetFnDestroy(TypeHandler::FnDestroy fnDestroy);
        void                    SetFnCopy(TypeHandler::FnCopy fnCopy);
        void                    SetFnDestructor(TypeHandler::FnDestructor destructor);
        void                    SetFnMove(TypeHandler::FnMove fnMove);

        ConstructorBuilder      NewConstructor(TypeID* ids, FieldInfo* params, usize size);
        FieldBuilder            NewField(const StringView& fieldName);
        FunctionBuilder         NewFunction(const FunctionHandlerCreation& creation);
        void                    AddBaseType(TypeID typeId, FnCast fnCast);

        TypeHandler& GetTypeHandler() const;

    private:
        TypeHandler& m_typeHandler;
    };

    ///-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    ///------------------------------------------------------------------------------Native Handlers----------------------------------------------------------------------------------------------------
    ///-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------


    template<typename Base, typename Derived>
    struct TypeCaster
    {
        static VoidPtr Cast(const TypeHandler* typeHandler, VoidPtr derived)
        {
            return static_cast<Base*>(static_cast<Derived*>(derived));
        }
    };

    template<typename Owner, typename Type>
    class NativeAttributeHandler
    {
    public:
        inline static Type Value{};

        NativeAttributeHandler(AttributeInfo& attributeInfo)
        {
            attributeInfo.GetValue = &GetValueImpl;
            attributeInfo.GetInfo = &GetTypeInfoImpl;
        }

    private:
        static ConstPtr GetValueImpl(const AttributeInfo* handler)
        {
            return &Value;
        }

        static TypeInfo GetTypeInfoImpl(const AttributeInfo* handler)
        {
            return GetTypeInfo<Type>();
        }
    };

    template<typename Owner, typename Type, typename Enable = void>
    struct NativeAttributeHandlerBuild
    {
        template<typename ...Args>
        static void Build(AttributeHandler& attributeHandler, Args&&... args)
        {
            AttributeBuilder builder {attributeHandler};
            NativeAttributeHandler<Owner, Type>::Value = Type{Traits::Forward<Args>(args)...};
            NativeAttributeHandler<Owner, Type>{builder.NewAttribute(GetTypeID<Type>())};

        }
    };

    template<typename Owner, typename Type>
    struct NativeAttributeHandlerBuild<Owner, Type, Traits::EnableIf<!Traits::IsComplete<Type>>>
    {
        template<typename ...Args>
        static void Build(AttributeHandler& attributeHandler, Args&&... args)
        {
        }
    };

    template<typename Owner>
    class NativeAttributeBuilder
    {
    public:
        explicit NativeAttributeBuilder(AttributeHandler& attributeHandler) : m_attributeHandler(attributeHandler)
        {
        }

        template<typename Type, typename ...Args>
        inline Owner& Attribute(Args&& ... args)
        {
            NativeAttributeHandlerBuild<Owner, Type>::Build(m_attributeHandler, Traits::Forward<Args>(args)...);
            return *static_cast<Owner*>(this);
        }
    private:
        AttributeHandler& m_attributeHandler;
    };


    template<typename Owner, typename ...Args>
    class NativeConstructorHandler
    {
    public:
        explicit NativeConstructorHandler(ConstructorBuilder constructorBuilder) : m_constructorHandler(constructorBuilder)
        {
            constructorBuilder.SetNewInstanceFn(&NewInstanceImpl);
            constructorBuilder.SetPlacementNewFn(PlacementNewImpl);
        }

    private:
        ConstructorBuilder m_constructorHandler;

        template<typename ...Vals>
        static void Eval(VoidPtr memory, VoidPtr* params, Traits::IndexSequence<>, Vals&& ...vals)
        {
            if constexpr (Traits::IsAggregate<Owner>)
            {
                new(PlaceHolder(), memory) Owner{Traits::Forward<Vals>(vals)...};
            }
            else
            {
                new(PlaceHolder(), memory) Owner(Traits::Forward<Vals>(vals)...);
            }
        }

        template<typename T, typename ...Tp, usize I, usize... Is, typename ...Vls>
        static void Eval(VoidPtr memory, VoidPtr* params, Traits::IndexSequence<I, Is...> seq, Vls&& ...vls)
        {
            return Eval<Tp...>(memory, params, Traits::IndexSequence<Is...>(), Traits::Forward<Vls>(vls)..., *static_cast<T*>(params[I]));
        }

        static void PlacementNewImpl(ConstructorHandler* handler, VoidPtr memory, VoidPtr* params)
        {
            const usize size = sizeof...(Args);
            Eval<Args...>(memory, params, Traits::MakeIntegerSequence<usize, size>{});
        }

        static VoidPtr NewInstanceImpl(ConstructorHandler* handler, Allocator& allocator, VoidPtr* params)
        {
            VoidPtr ptr = allocator.MemAlloc(sizeof(Owner), alignof(Owner));
            PlacementNewImpl(handler, ptr, params);
            return ptr;
        }
    };


    //fields

    template<auto mfp, typename Owner, typename Field>
    class NativeFieldHandlerBase
    {
    protected:
        explicit NativeFieldHandlerBase(FieldBuilder fieldBuilder)
        {
            fieldBuilder.SetFnGetFieldInfo(&GetFieldImpl);
            fieldBuilder.SetFnGetFieldPointer(&FnGetFieldPointerImpl);
        }

        static FieldInfo GetFieldImpl(const FieldHandler* fieldHandler)
        {
            return MakeFieldInfo<Owner, Field>();
        }

        static VoidPtr FnGetFieldPointerImpl(const FieldHandler* fieldHandler, VoidPtr instance)
        {
            return &((*static_cast<Owner*>(instance)).*mfp);
        }
    };

    template<auto mfp, auto getFp, auto setFp, typename Owner, typename Field>
    class NativeFieldHandler : NativeFieldHandlerBase<mfp, Owner, Field>
    {
    public:
        explicit NativeFieldHandler(FieldBuilder fieldBuilder): NativeFieldHandlerBase<mfp, Owner, Field>(fieldBuilder)
        {
            fieldBuilder.SetFnSetValue(&SetValue);
            fieldBuilder.SetFnGetValue(&GetValue);
        }
    private:
        static void GetValue(const FieldHandler* fieldHandler, VoidPtr instance, VoidPtr result)
        {
            using Return = Traits::FunctionReturnType<getFp>;
            *static_cast<Traits::RemoveReference<Return>*>(result) = (*static_cast<Owner*>(instance).*getFp)();

        }

        static void SetValue(const FieldHandler* fieldHandler, VoidPtr instance, ConstPtr value)
        {
            (*static_cast<Owner*>(instance).*setFp)(*static_cast<const Field*>(value));
        }
    };

    template <auto mfp, typename Owner, typename Field>
    class NativeFieldHandler<mfp, nullptr, nullptr, Owner, Field> : NativeFieldHandlerBase<mfp, Owner, Field>
    {
    public:
        explicit NativeFieldHandler(FieldBuilder fieldBuilder): NativeFieldHandlerBase<mfp, Owner, Field>(fieldBuilder)
        {
            fieldBuilder.SetFnCopyValueTo(&CopyValueToImpl);
            fieldBuilder.SetFnSetValue(&SetValue);
        }
    private:
        static void CopyValueToImpl(const FieldHandler* fieldHandler, ConstPtr instance, VoidPtr value)
        {
            *static_cast<Field*>(value) = ((*static_cast<const Owner*>(instance)).*mfp);
        }

        static void SetValue(const FieldHandler* fieldHandler, VoidPtr instance, ConstPtr value)
        {
            *static_cast<Owner*>(instance).*mfp = *static_cast<const Field*>(value);
        }
    };


    template<auto mfp, auto getFp, auto setFp, typename Field>
    struct FieldTemplateDecomposer
    {
    };

    template<auto mfp, auto getFp, auto setFp, typename Owner, typename FieldType>
    struct FieldTemplateDecomposer<mfp, getFp, setFp, FieldType Owner::*>
    {

        static TypeID GetTypeId()
        {
            return GetTypeID<FieldType>();
        }

        static auto CreateHandler(FieldBuilder fieldBuilder)
        {
            return NativeFieldHandler<mfp, getFp, setFp, Owner, FieldType>(fieldBuilder);
        }
    };


    //functions

    template<auto fp, typename Return, typename ...Args>
    class NativeFunctionHandler : public NativeAttributeBuilder<NativeFunctionHandler<fp, Return, Args...>>
    {
    public:
        NativeFunctionHandler(FunctionBuilder& functionBuilder) :
            NativeAttributeBuilder<NativeFunctionHandler<fp, Return, Args...>>(functionBuilder.GetFunctionHandler())
        {
            functionBuilder.SetFnInvoke(InvokeImpl);
            functionBuilder.SetFunctionPointer(reinterpret_cast<VoidPtr>(&FunctionImpl));
        }

    private:
        static void InvokeImpl(const FunctionHandler* handler, VoidPtr instance, VoidPtr  ret, VoidPtr* params)
        {
            u32 i{sizeof...(Args)};
            if constexpr (Traits::IsSame<Return, void>)
            {
                fp(*static_cast<Traits::RemoveReference<Args>*>(params[--i])...);
            }
            else
            {
                *static_cast<Traits::RemoveReference<Return>*>(ret) = fp(*static_cast<Traits::RemoveReference<Args>*>(params[--i])...);
            }
        }

        static Return FunctionImpl(const FunctionHandler* handler, VoidPtr  instance, Args... args)
        {
            return fp(args...);
        }
    };

    template<auto mfp, typename Return, typename Owner, typename ...Args>
    class NativeMemberFunctionHandler : public NativeAttributeBuilder<NativeMemberFunctionHandler<mfp, Return, Owner, Args...>>
    {
    private:
        FunctionBuilder m_functionBuilder;
    public:
        explicit NativeMemberFunctionHandler(FunctionBuilder& functionBuilder) : NativeAttributeBuilder<NativeMemberFunctionHandler<mfp, Return, Owner, Args...>>(functionBuilder.GetFunctionHandler()),
            m_functionBuilder(functionBuilder)
        {
            functionBuilder.SetFnInvoke(InvokeImpl);
            functionBuilder.SetFunctionPointer(reinterpret_cast<VoidPtr>(&FunctionImpl));
        }
    private:
        static void InvokeImpl(const FunctionHandler* handler, VoidPtr instance, VoidPtr ret, VoidPtr * params)
        {
            u32 i{sizeof...(Args)};
            if constexpr (Traits::IsSame<Return,void>)
            {
                (static_cast<Owner*>(instance)->*mfp)(*static_cast<Traits::RemoveReference<Args>*>(params[--i])...);
            }
            else
            {
                *static_cast<Traits::RemoveReference<Return>*>(ret) = (static_cast<Owner*>(instance)->*mfp)(*static_cast<Traits::RemoveReference<Args>*>(params[--i])...);
            }
        }

        static Return FunctionImpl(const FunctionHandler* handler, VoidPtr instance, Args... args)
        {
            return (static_cast<Owner*>(instance)->*mfp)(args...);
        }
    };

    template<auto func, typename Function>
    struct MemberFunctionTemplateDecomposer
    {
    };

    template<auto mfp, typename Return, typename Owner>
    struct MemberFunctionTemplateDecomposer<mfp, Return(Owner::*)()>
    {
        using FuncType = MemberFunctionTemplateDecomposer<mfp, Return(Owner::*)()>;

        static decltype(auto) CreateHandler(FunctionBuilder functionBuilder)
        {
            return NativeMemberFunctionHandler<mfp, Return, Owner>(functionBuilder);
        }

        static FunctionHandlerCreation MakeCreation(const StringView& name)
        {
            return FunctionHandlerCreation{
                .name = name,
                .functionId = GetTypeID<FuncType>(),
                .owner = GetTypeID<Owner>(),
                .retInfo = MakeFieldInfo<Owner, Return>()
            };
        }
    };

    template<auto mfp, typename Return, typename Owner, typename ...Args>
    struct MemberFunctionTemplateDecomposer<mfp, Return(Owner::*)(Args...)>
    {
        using FuncType = MemberFunctionTemplateDecomposer<mfp, Return(Owner::*)()>;
        static constexpr FieldInfo Params[] = {MakeFieldInfo<Owner, Args>()...};

        static decltype(auto) CreateHandler(FunctionBuilder functionBuilder)
        {
            return NativeMemberFunctionHandler<mfp, Return, Owner, Args...>(functionBuilder);
        }

        static FunctionHandlerCreation MakeCreation(const StringView& name)
        {
            return FunctionHandlerCreation{
                .name = name,
                .functionId = GetTypeID<FuncType>(),
                .owner = GetTypeID<Owner>(),
                .params = {Params, sizeof...(Args)},
                .retInfo = MakeFieldInfo<Owner, Return>()
            };
        }
    };

    template<auto fp, typename Return>
    struct MemberFunctionTemplateDecomposer<fp, Return(*)()>
    {
        using FuncType = MemberFunctionTemplateDecomposer<fp, Return(*)()>;

        static decltype(auto) CreateHandler(FunctionBuilder functionBuilder)
        {
            return NativeFunctionHandler<fp, Return>(functionBuilder);
        }

        static FunctionHandlerCreation MakeCreation(const StringView& name)
        {
            return FunctionHandlerCreation{
                .name = name,
                .functionId = GetTypeID<FuncType>(),
                .retInfo = MakeFieldInfo<void, Return>()
            };
        }
    };

    template<auto fp, typename Return, typename ...Args>
    struct MemberFunctionTemplateDecomposer<fp, Return(*)(Args...)>
    {
        using FuncType = MemberFunctionTemplateDecomposer<fp, Return(*)(Args...)>;
        static constexpr FieldInfo Params[] = {MakeFieldInfo<void, Args>()...};

        static decltype(auto) CreateHandler(FunctionBuilder functionBuilder)
        {
            return NativeFunctionHandler<fp, Return, Args...>(functionBuilder);
        }

        static FunctionHandlerCreation MakeCreation(const StringView& name)
        {
            return FunctionHandlerCreation{
                .name = name,
                .functionId = GetTypeID<FuncType>(),
                .params = {Params, sizeof...(Args)},
                .retInfo = MakeFieldInfo<void, Return>()
            };
        }
    };


    //type

    template<typename Type, typename Enabler = void>
    struct NativeTypeHandlerFuncs
    {
        static void DestroyImpl(const TypeHandler* typeHandler, Allocator& allocator, VoidPtr instance){};
        static void CopyImpl(const TypeHandler* typeHandler, ConstPtr source, VoidPtr dest) {};
        static void DestructorImpl(const TypeHandler* typeHandler, VoidPtr instance){};
        static void MoveImpl(const TypeHandler* typeHandler, VoidPtr origin, VoidPtr destination) {};
    };

    template<typename Type>
    struct NativeTypeHandlerFuncs<Type, Traits::VoidType<decltype(sizeof(Type) != 0)>>
    {
        static void DestroyImpl(const TypeHandler* typeHandler, Allocator& allocator, VoidPtr instance)
        {
            if constexpr (Traits::IsDestructible<Type>)
            {
                static_cast<Type*>(instance)->~Type();
            }
            allocator.MemFree(instance);
        }

        static void CopyImpl(const TypeHandler* typeHandler, ConstPtr source, VoidPtr dest)
        {
            if constexpr (Traits::IsCopyConstructible<Type>)
            {
                new(PlaceHolder(), dest) Type(*static_cast<const Type*>(source));
            }
        }

        static void DestructorImpl(const TypeHandler* typeHandler, VoidPtr instance)
        {
            if constexpr (Traits::IsDestructible<Type>)
            {
                static_cast<Type*>(instance)->~Type();
            }
        };

        static void MoveImpl(const TypeHandler* typeHandler, VoidPtr origin, VoidPtr destination)
        {
            if constexpr (Traits::IsMoveConstructible<Type>)
            {
                new(PlaceHolder(), destination) Type{std::move(*static_cast<Type*>(origin))};
            }
            else if constexpr (Traits::IsCopyConstructible<Type>)
            {
                new(PlaceHolder(), destination) Type{*static_cast<const Type*>(origin)};
            }
            else
            {
                FY_ASSERT(false, "type is not move or copy constructible");
            }
        }
    };


    template<typename Type>
    class NativeTypeHandler : public NativeAttributeBuilder<NativeTypeHandler<Type>>
    {
    private:
        TypeBuilder m_typeBuilder;
    public:
        explicit NativeTypeHandler(TypeBuilder typeBuilder) : NativeAttributeBuilder<NativeTypeHandler<Type>>(typeBuilder.GetTypeHandler()), m_typeBuilder(typeBuilder)
        {
            if constexpr (Traits::IsDirectConstructible<Type>)
            {
                this->Constructor();
            }
            typeBuilder.SetFnDestroy(&NativeTypeHandlerFuncs<Type>::DestroyImpl);
            typeBuilder.SetFnCopy(&NativeTypeHandlerFuncs<Type>::CopyImpl);
            typeBuilder.SetFnDestructor(&NativeTypeHandlerFuncs<Type>::DestructorImpl);
            typeBuilder.SetFnMove(&NativeTypeHandlerFuncs<Type>::MoveImpl);
        }

        auto Constructor()
        {
            return NativeConstructorHandler<Type>(m_typeBuilder.NewConstructor(nullptr, nullptr, 0));
        }

        template<typename ...Args>
        auto Constructor()
        {
            FieldInfo params[] = {MakeFieldInfo<void, Args>()...};
            TypeID ids[] = {GetTypeID<Args>()...,};
            return NativeConstructorHandler<Type, Args...>(m_typeBuilder.NewConstructor(ids, params, sizeof...(Args)));
        }

        template<auto mfp>
        auto Field(const StringView& name)
        {
            using FieldDecomp = FieldTemplateDecomposer<mfp, nullptr, nullptr, decltype(mfp)>;
            return FieldDecomp::CreateHandler(m_typeBuilder.NewField(name));
        }

        template<auto mfp, auto getFp, auto setFp>
        auto Field(const StringView& fieldName)
        {
            using FieldDecomp = FieldTemplateDecomposer<mfp, getFp, setFp, decltype(mfp)>;
            return FieldDecomp::CreateHandler(m_typeBuilder.NewField(fieldName));
        }


        template<auto mfp>
        auto Function(const StringView& name)
        {
            using FuncType = Traits::RemoveConstFunc<decltype(mfp)>;
            using DecompType = MemberFunctionTemplateDecomposer<mfp, FuncType>;
            return DecompType::CreateHandler(m_typeBuilder.NewFunction(DecompType::MakeCreation(name)));
        }
    };

    template<typename, typename = void>
    struct HasRegisterTypeImpl : Traits::FalseType
    {
    };

    template<typename T>
    struct HasRegisterTypeImpl<T, Traits::VoidType<decltype(static_cast<void(*)(NativeTypeHandler<T>&)>(&T::RegisterType))>> : Traits::TrueType
    {
    };

    template<typename T>
    constexpr bool HasRegisterType = HasRegisterTypeImpl<T>::value;

    namespace Registry
    {
        FY_API TypeBuilder      NewType(const StringView& name, const TypeInfo& typeInfo);
        FY_API TypeHandler*     FindTypeByName(const StringView& name);
        FY_API TypeHandler*     FindTypeById(TypeID typeId);


        FY_API FunctionBuilder          NewFunction(const FunctionHandlerCreation& functionHandlerCreation);
        FY_API FunctionHandler*         FindFunctionByName(const StringView& name);
        FY_API Span<FunctionHandler*>   FindFunctionsByAttribute(TypeID typeId);


        template<typename T, typename ...B>
        inline decltype(auto) Type(const StringView& name)
        {
            TypeBuilder typeBuilder = NewType(name, GetTypeInfo<T>());
            (typeBuilder.AddBaseType(GetTypeID<B>(), TypeCaster<B, T>::Cast),...);

            NativeTypeHandler<T> handler = NativeTypeHandler<T>(NewType(name, GetTypeInfo<T>()));

            if constexpr (HasRegisterType<T>)
            {
                T::RegisterType(handler);
            }

            return handler;
        }

        template<typename T, typename ...B>
        inline decltype(auto) Type()
        {
            return Type<T, B...>(GetTypeName<T>());
        }

        template<typename T>
        TypeHandler* FindType()
        {
            return FindTypeById(GetTypeID<T>());
        }

        template<typename T>
        Span<FunctionHandler*> FindFunctionsByAttribute()
        {
            return FindFunctionsByAttribute(GetTypeID<T>());
        }

        template<auto Func>
        inline decltype(auto) Function(const StringView& name)
        {
            using FuncType = Traits::RemoveConstFunc<decltype(Func)>;
            using DecompType = MemberFunctionTemplateDecomposer<Func, FuncType>;
            return DecompType::CreateHandler(NewFunction(DecompType::MakeCreation(name)));
        }
    }
}