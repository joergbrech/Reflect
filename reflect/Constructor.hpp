#ifndef CONSTRUCTOR_H
#define CONSTRUCTOR_H

#include <vector>
#include <tuple>
#include <any>

#include "TypeDescriptor.hpp"
#include "Conversion.hpp"
#include "Base.hpp"

namespace Reflect
{

	inline bool CanCastOrConvert(const TypeDescriptor *from, const TypeDescriptor *to)
	{
		if (from == to)
			return true;

		for (auto *base : from->GetBases())
			if (base->GetType() == to)
				return true;

		for (auto *conversion : from->GetConversions())
			if (conversion->GetToType() == to)
				return true;

		return false;
	}

	class Constructor
	{
	public:
		std::any NewInstance(std::vector<std::any> &args)
		{
			if (args.size() == mParamTypes.size())
				return NewInstanceImpl(args);

			return std::any();
		}

		template <typename... Args>
		std::any NewInstance(Args&&... args) const
		{
			if (sizeof...(Args) == mParamTypes.size())
			{
				auto argsAny = std::vector<std::any>({ std::any(std::forward<Args>(args))... });
				return NewInstanceImpl(argsAny);
			}

			return std::any();
		}

		TypeDescriptor const *GetParent() const
		{
			return mParent;
		}

		TypeDescriptor const *GetParamType(size_t index) const
		{
			return mParamTypes[index];
		}

		size_t GetNumParams() const
		{
			return mParamTypes.size();
		}

		template <typename... Args, size_t... indices>
		bool CanConstruct(std::index_sequence<indices...> indexSequence = std::index_sequence_for<Args...>()) const
		{
			return GetNumParams() == sizeof...(Args) && ((Reflect::CanCastOrConvert(Details::Resolve<Args>(), GetParamType(indices))) && ...);
		}

	protected:
		Constructor(TypeDescriptor *parent, const std::vector<const TypeDescriptor*> &paramTypes) : mParent(parent), mParamTypes(paramTypes) {}

	private:
		virtual std::any NewInstanceImpl(std::vector<std::any> &args) const = 0;

		TypeDescriptor *mParent;
		std::vector<TypeDescriptor const*> mParamTypes;
	};

	template <typename Type, typename... Args>
	class ConstructorImpl : public Constructor
	{
	public:
		ConstructorImpl() : Constructor(Details::Resolve<Details::RawType<Type>>(), { Details::Resolve<Details::RawType<Args>>()... }) {}

	private:
		std::any NewInstanceImpl(std::vector<std::any> &args) const override
		{
			return NewInstanceImpl(args, std::make_index_sequence<sizeof...(Args)>());
		}

		template <size_t... indices>
		std::any NewInstanceImpl(std::vector<std::any> &args, std::index_sequence<indices...> indexSequence) const
		{
			std::tuple argsTuple = std::make_tuple(std::any_cast<std::remove_cv_t<std::remove_reference_t<Args>>>(&args[indices])...);
			std::vector<std::any> convertedArgs{ std::get<indices>(argsTuple)... };
			argsTuple = std::make_tuple(std::any_cast<std::remove_cv_t<std::remove_reference_t<Args>>>(&convertedArgs[indices])...);

			if ((std::get<indices>(argsTuple) && ...))
				return Type(*std::get<indices>(argsTuple)...);
			
			return std::any();
		}
	};

	template <typename Type, typename... Args>
	class FreeFunConstructor : public Constructor
	{
	private:
		typedef Type(*CtorFun)(Args...);
	public:
		FreeFunConstructor(CtorFun ctorFun) : Constructor(Details::Resolve<Details::RawType<Type>>(), { Details::Resolve<Details::RawType<Args>>()... }), mCtorFun(ctorFun) {}
	
	private:
		std::any NewInstanceImpl(std::vector<std::any> &args) const override
		{
			return NewInstanceImpl(args, std::make_index_sequence<sizeof...(Args)>());
		}

		template <size_t... indices>
		std::any NewInstanceImpl(std::vector<std::any> &args, std::index_sequence<indices...> indexSequence) const
		{
			std::tuple argsTuple = std::make_tuple(std::any_cast<std::remove_cv_t<std::remove_reference_t<Args>>>(&args[indices])...);
			std::vector<std::any> convertedArgs{ std::get<indices>(argsTuple)... };
			argsTuple = std::make_tuple(std::any_cast<std::remove_cv_t<std::remove_reference_t<Args>>>(&convertedArgs[indices])...);

			if ((std::get<indices>(argsTuple) && ...))
				return mCtorFun(*std::get<indices>(argsTuple)...);

			return std::any();
		}
		
		CtorFun mCtorFun;
	};

}  // namespace Reflect

#endif // CONSTRUCTOR_H
