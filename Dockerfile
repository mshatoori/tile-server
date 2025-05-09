# Use a base image with needed build tools and libraries
# Ubuntu 22.04 (Jammy) has reasonably recent Mapnik (3.1.x) and Boost (1.74)
FROM ubuntu:22.04 AS builder

# Avoid interactive prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive

# Install build dependencies
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    git \
    # Mapnik core library and development headers
    libmapnik-dev \
    pkg-config \
    # Boost libraries required by our code and potentially Mapnik
    libboost-system-dev \
    libboost-thread-dev \
    libboost-filesystem-dev \
    libboost-program-options-dev \
    libboost-date-time-dev \
    # Runtime dependencies for libmapnik (libmapnik-dev should pull most, but safer to list key ones)
    # Check `ldd /usr/lib/libmapnik.so` on the image if needed
    libboost-regex1.74.0 \
    libcairo2 \
    libicu70 \
    libproj22 \
    libxml2 \
    libjpeg-turbo8 \
    libpng16-16 \
    libtiff5 \
    libwebp7 \
    zlib1g \
    # Required by the osm plugin (protobuf, zlib, bzip2)
    libprotobuf-dev \
    protobuf-compiler \
    zlib1g-dev \
    libbz2-dev \
    && rm -rf /var/lib/apt/lists/*

    
# Set working directory
WORKDIR /app

# Copy source code
COPY src ./src
COPY styles ./styles
COPY Makefile .

# Configure and build the application
RUN make --jobs=$(nproc)

# --- Final Stage ---
# Use a smaller base image if possible, but ensure all runtime dependencies of Mapnik are present
FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

# Install only runtime dependencies
# Note: This list needs to exactly match the runtime needs of the compiled binary and libmapnik
RUN apt-get update && apt-get install -y --no-install-recommends \
    libmapnik3.1 \
    libboost-system1.74.0 \
    libboost-thread1.74.0 \
    libboost-filesystem1.74.0 \
    libboost-program-options1.74.0 \
    libboost-date-time1.74.0 \
    libboost-regex1.74.0 \
    libcairo2 \
    libicu70 \
    libproj22 \
    libxml2 \
    libjpeg-turbo8 \
    libpng16-16 \
    libtiff5 \
    libwebp7 \
    zlib1g \
    libprotobuf-lite23 \
    libbz2-1.0 \
    # Ensure Mapnik input plugins are installed
    # mapnik-input-plugin-osm \
    # mapnik-input-plugin-postgis \
    # mapnik-input-plugin-shape \
    # Ensure fonts are available for labels (if your style uses them)
    fonts-dejavu \
    fonts-noto-cjk \
    fonts-noto-hinted \
    fonts-noto-unhinted \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copy built executable from builder stage
COPY --from=builder /app/osm_mapnik_server /app/osm_mapnik_server
# Copy the style file
COPY --from=builder /app/styles/basic_style.xml /app/styles/basic_style.xml

# Create a directory for map data
VOLUME /data
WORKDIR /data

# Expose the server port
EXPOSE 8080

# Default command to run the server
# Expects the PBF file to be mounted at /data/map.pbf
CMD ["/app/osm_mapnik_server", "--pbf_file", "/data/map.pbf", "--style_file", "/app/styles/basic_style.xml", "--port", "8080"]