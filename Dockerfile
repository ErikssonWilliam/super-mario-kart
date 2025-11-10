FROM ubuntu:24.04

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    libboost-all-dev \
    libsfml-dev \
    x11-apps \
    xauth \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . .

# Build main game WITH audio (just link the library)
WORKDIR /app/src
RUN make clean && make release

# Build networked client WITH audio library
WORKDIR /app
RUN g++ -std=c++17 \
    -DIN_DOCKER \
    $(find bin -name "*.o" -not -name "main.o") \
    webserver/networked_main.cpp \
    -o networked_client \
    -I src \
    -lboost_system -lboost_thread -lpthread \
    -lsfml-graphics -lsfml-window -lsfml-system -lsfml-audio 

# Copy and make executable the wrapper script

ENV DISPLAY=:0
CMD ["./networked_client"]


#docker build -t mario-kart-client:latest .

#Allow docker to show the graphics
#xhost +

#docker run -it --rm   --network ai-grand-prix_hackathon-net   -v /tmp/.X11-unix:/tmp/.X11-unix:rw   -e DISPLAY=:0   mario-kart-client:latest