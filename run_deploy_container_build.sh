#!/bin/bash

PATH_LOCAL_DEPLOY="deploy"
PATH_CONTAINER_BUILD="/project/build"

CONTAINER_NAME="sensornode_rp2040"

# clean deploy folder
rm -f $PATH_LOCAL_DEPLOY/sensornode_rp2040.uf2

echo "Run docker container"
docker run --name $CONTAINER_NAME -d sensornode:latest
sleep 5
docker cp $CONTAINER_NAME:"$PATH_CONTAINER_BUILD/sensornode_rp2040.uf2" "$PATH_LOCAL_DEPLOY/sensornode_rp2040.uf2"

docker stop $CONTAINER_NAME
docker rm $CONTAINER_NAME

echo "Script finished"