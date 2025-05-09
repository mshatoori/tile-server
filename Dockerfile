# Use a suitable Node.js base image
FROM node:lts

# Set the working directory
WORKDIR /app

# Copy package.json and package-lock.json (if it exists)
COPY package*.json ./

# Install Node.js dependencies
RUN npm install

# Copy the rest of the application files
COPY server.js ./
COPY renderer.js ./
COPY styles/ ./styles/

# Expose the port the Node.js server listens on
EXPOSE 3000

# Define the command to start the Node.js server
CMD [ "npm", "start" ]