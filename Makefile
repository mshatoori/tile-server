# Makefile for Node.js Tile Server

# Define the Docker image name
IMAGE_NAME = tile-server

# Build the Docker image
build:
	docker build -t $(IMAGE_NAME) .

# Run the Docker container (example)
run:
	docker run -p 3000:3000 $(IMAGE_NAME)

# Clean up (optional)
clean:
	# Add any cleanup commands here if needed
	@echo "No cleanup commands defined yet."

.PHONY: build run clean