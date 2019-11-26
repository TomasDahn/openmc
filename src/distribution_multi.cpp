#include "openmc/distribution_multi.h"

#include <algorithm> // for move
#include <cmath>     // for sqrt, sin, cos, max

#include "openmc/constants.h"
#include "openmc/error.h"
#include "openmc/math_functions.h"
#include "openmc/random_lcg.h"
#include "openmc/xml_interface.h"

namespace openmc {

//==============================================================================
// UnitSphereDistribution implementation
//==============================================================================

UnitSphereDistribution::UnitSphereDistribution(pugi::xml_node node)
{
  // Read reference directional unit vector
  if (check_for_node(node, "reference_uvw")) {
    auto u_ref = get_node_array<double>(node, "reference_uvw");
    if (u_ref.size() != 3)
      fatal_error("Angular distribution reference direction must have "
                  "three parameters specified.");
    u_ref_ = Direction(u_ref.data());
  }
}


//==============================================================================
// PolarAzimuthal implementation
//==============================================================================

PolarAzimuthal::PolarAzimuthal(Direction u, UPtrDist mu, UPtrDist phi) :
  UnitSphereDistribution{u}, mu_{std::move(mu)}, phi_{std::move(phi)} { }

PolarAzimuthal::PolarAzimuthal(pugi::xml_node node)
  : UnitSphereDistribution{node}
{
  if (check_for_node(node, "mu")) {
    pugi::xml_node node_dist = node.child("mu");
    mu_ = distribution_from_xml(node_dist);
  } else {
    mu_ = UPtrDist{new Uniform(-1., 1.)};
  }

  if (check_for_node(node, "phi")) {
    pugi::xml_node node_dist = node.child("phi");
    phi_ = distribution_from_xml(node_dist);
  } else {
    phi_ = UPtrDist{new Uniform(0.0, 2.0*PI)};
  }
}

Direction PolarAzimuthal::sample(uint64_t * prn_seeds, int stream) const
{
  // Sample cosine of polar angle
  double mu = mu_->sample(prn_seeds, stream);
  if (mu == 1.0) return u_ref_;

  // Sample azimuthal angle
  double phi = phi_->sample(prn_seeds, stream);

  // If the reference direction is along the z-axis, rotate the aziumthal angle
  // to match spherical coordinate conventions.
  // TODO: apply this change directly to rotate_angle
  if (u_ref_.x == 0 && u_ref_.y == 0) phi += 0.5*PI;

  return rotate_angle(u_ref_, mu, &phi, prn_seeds, stream);
}

//==============================================================================
// Isotropic implementation
//==============================================================================

Direction Isotropic::sample(uint64_t * prn_seeds, int stream) const
{
  double phi = 2.0*PI*prn(prn_seeds, stream);
  double mu = 2.0*prn(prn_seeds, stream) - 1.0;
  return {mu, std::sqrt(1.0 - mu*mu) * std::cos(phi),
      std::sqrt(1.0 - mu*mu) * std::sin(phi)};
}

//==============================================================================
// Monodirectional implementation
//==============================================================================

Direction Monodirectional::sample(uint64_t * prn_seeds, int stream) const
{
  return u_ref_;
}

} // namespace openmc
