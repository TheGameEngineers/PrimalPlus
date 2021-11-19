// NOTE: put this inside the Utilities.h files in Primal. It defines
//       _countof() for non Windows systems.

#ifndef _WIN64
template < typename T, size_t N >
size_t constexpr _countof( T ( & arr )[ N ] )
{
    return std::extent< T[ N ] >::value;
}
#endif // !_WIN64