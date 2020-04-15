\MIX = mix
CFLAGS = -O3 -std=c99

PREFIX=$(MIX_APP_PATH)/priv

ifndef MIX_ENV
	MIX_ENV = dev
endif

ifdef DEBUG
	CFLAGS +=  -pedantic -Weverything -Wall -Wextra -Wno-unused-parameter -Wno-gnu
endif

ifeq ($(MIX_ENV),dev)
	CFLAGS += -g
endif

LDFLAGS += `pkg-config --static --libs glfw3 glew`
CFLAGS += `pkg-config --static --cflags glfw3 glew`


ifneq ($(OS),Windows_NT)
	CFLAGS += -fPIC

	ifeq ($(shell uname),Darwin)
		LDFLAGS += -framework Cocoa -framework OpenGL -Wno-deprecated
	else
	  LDFLAGS += -lGL -lm -lrt
	endif
endif



.PHONY: all clean

all: $(PREFIX)/$(MIX_ENV)/scenic_driver_glfw
# fonts

SRCS = c_src/main.c c_src/comms.c c_src/nanovg/nanovg.c \
	c_src/utils.c c_src/render_script.c c_src/tx.c
	# c_src/nanovg/nanovg.c
	# c_src/render.c c_src/text.c c_src/texture.c

$(PREFIX)/$(MIX_ENV)/scenic_driver_glfw: $(SRCS)
	mkdir -p $(PREFIX)/$(MIX_ENV)
	$(CC) $(CFLAGS) -o $@ $(SRCS) $(LDFLAGS)

# fonts: priv/
# 	ln -fs ../fonts priv/

clean:
	$(RM) -r $(PREFIX)/$(MIX_ENV)
#	$(RM) -r $(PREFIX)/test
#	$(RM) -r $(PREFIX)/prod
