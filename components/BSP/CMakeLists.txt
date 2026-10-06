set(src_dirs
			IIC
			AS5600)

set(include_dirs
			IIC
			AS5600)

set(requires
			driver)

idf_component_register(SRC_DIRS ${src_dirs}
					  INCLUDE_DIRS ${include_dirs} REQUIRES ${requires})

