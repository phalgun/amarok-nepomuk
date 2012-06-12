find_package( Nepomuk REQUIRED )

include_directories( ../..

   ${NEPOMUK_INCLUDES}
)

########### next target ###############

set(amarok_collection-nepomukcollection_PART_SRCS
   NepomukCollection.cpp
   NepomukCollectionFactory.cpp
)
kde4_add_plugin(amarok_collection-nepomukcollection ${amarok_collection-nepomukcollection_PART_SRCS})

target_link_libraries(amarok_collection-nepomukcollection
    amarokcore
    amaroklib
    ${NEPOMUK_LIBRARIES}
)

install(TARGETS amarok_collection-nepomukcollection DESTINATION ${PLUGIN_INSTALL_DIR} )

########### install files ###############

install( FILES amarok_collection-nepomukcollection.desktop DESTINATION ${SERVICES_INSTALL_DIR})
