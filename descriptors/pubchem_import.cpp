
#include <iostream>

#include <mongo/client/dbclient.h>

#include <chemkit/atom.h>
#include <chemkit/foreach.h>
#include <chemkit/molecule.h>
#include <chemkit/moleculefile.h>

using namespace mongo;
using namespace chemkit;

int main(int argc, char *argv[])
{
  if(argc < 2){
    std::cerr << "Usage: " << argv[0] << " file.sdf" << std::endl;
    return -1;
  }

  // connect to mongo database
  mongo::DBClientConnection db;
  std::string host = "localhost";
  try {
    db.connect(host);
    std::cout << "Connected to: " << host << std::endl;
  }
  catch (DBException &e) {
    std::cerr << "Failed to connect to MongoDB: " << e.what() << std::endl;
  }
  
  // read pubchem file
  MoleculeFile file(argv[1]);
  bool ok = file.read();
  if(!ok){
    std::cerr << "Failed to read file: " << file.errorString() << std::endl;
  }

  // drop the current molecules collection
  db.dropCollection("chem.molecules");

  // index on inchikey
  db.ensureIndex("chem.molecules", BSON("inchikey" << 1), /* unique = */ true);

  // index on heavy atom count
  db.ensureIndex("chem.molecules", BSON("heavyAtomCount" << 1), /* unique = */ false);

  size_t count = 0;
  foreach(const boost::shared_ptr<Molecule> &molecule, file.molecules()){
    std::string name = molecule->data("PUBCHEM_IUPAC_TRADITIONAL_NAME").toString();
    std::string formula = molecule->formula();
    std::string inchi = molecule->data("PUBCHEM_IUPAC_INCHI").toString();
    std::string inchikey = molecule->data("PUBCHEM_IUPAC_INCHIKEY").toString();
    double mass = molecule->data("PUBCHEM_MOLECULAR_WEIGHT").toDouble();
    int atomCount = molecule->atomCount();
    int heavyAtomCount = molecule->atomCount() - molecule->atomCount(Atom::Hydrogen);

    OID id = OID::gen();

    db.insert("chem.molecules",
              BSON("_id" << id <<
                   "name" << name <<
                   "formula" << formula <<
                   "inchi" << inchi <<
                   "inchikey" << inchikey <<
                   "mass" << mass <<
                   "atomCount" << atomCount <<
                   "heavyAtomCount" << heavyAtomCount
              ));

    // only import the first 5000 molecules
    if(count++ > 5000){
      break;
    }
  }

  return 0;
}
