#!/usr/bin/env bash
echo "Creating SO2 container..."
docker run -d -it -v $(pwd):/root/workspace -w /root/workspace --rm -p 2150:22 --name so2 neoxelox/so2:latest
echo "Container created! Waiting for IP assignment..."
sleep 10
CONTAINER_IP=$(docker inspect -f '{{range .NetworkSettings.Networks}}{{.IPAddress}}{{end}}' so2)
echo "IP captured! SSHing..."
ssh -X root@"${CONTAINER_IP}"
echo "Sesion stopped. Stopping container..."
docker stop so2
