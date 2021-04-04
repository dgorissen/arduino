# free up some space
sudo apt-get purge wolfram-engine
sudo apt-get purge libreoffice
sudo apt-get clean
sudo apt-get autoremove

# install updates
sudo apt-get update && sudo apt-get upgrade

# setup git
git config --global user.email "dgorissen@gmail.com"
git config --global user.name "Dirk Gorissen"

# get std packages
sudo apt install -y vim keychain imagemagick feh screen

# install docker
curl -fsSL https://get.docker.com -o get-docker.sh
sudo sh get-docker.sh
sudo usermod -aG docker dgorissen

# allos serial access
sudo usermod -a -G dialout dgorissen

# add this to bashrc
eval $(keychain --noask --eval id_rsa)

# if docker img does not build on the pi because of key problems with apt, see
# https://askubuntu.com/questions/1263284/apt-update-throws-signature-error-in-ubuntu-20-04-container-on-arm