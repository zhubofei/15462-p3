/**
 * @file triangle.cpp
 * @brief Function definitions for the Triangle class.
 *
 * @author Eric Butler (edbutler)
 */

#include "scene/triangle.hpp"
#include "application/opengl.hpp"
#include "math/math.hpp"

namespace _462 {

Triangle::Triangle()
{
    vertices[0].material = 0;
    vertices[1].material = 0;
    vertices[2].material = 0;
    isBig=true;
}

Triangle::~Triangle() { }

void Triangle::render() const
{
    bool materials_nonnull = true;
    for ( int i = 0; i < 3; ++i )
        materials_nonnull = materials_nonnull && vertices[i].material;

    // this doesn't interpolate materials. Ah well.
    if ( materials_nonnull )
        vertices[0].material->set_gl_state();

    glBegin(GL_TRIANGLES);

#if REAL_FLOAT
    glNormal3fv( &vertices[0].normal.x );
    glTexCoord2fv( &vertices[0].tex_coord.x );
    glVertex3fv( &vertices[0].position.x );

    glNormal3fv( &vertices[1].normal.x );
    glTexCoord2fv( &vertices[1].tex_coord.x );
    glVertex3fv( &vertices[1].position.x);

    glNormal3fv( &vertices[2].normal.x );
    glTexCoord2fv( &vertices[2].tex_coord.x );
    glVertex3fv( &vertices[2].position.x);
#else
    glNormal3dv( &vertices[0].normal.x );
    glTexCoord2dv( &vertices[0].tex_coord.x );
    glVertex3dv( &vertices[0].position.x );

    glNormal3dv( &vertices[1].normal.x );
    glTexCoord2dv( &vertices[1].tex_coord.x );
    glVertex3dv( &vertices[1].position.x);

    glNormal3dv( &vertices[2].normal.x );
    glTexCoord2dv( &vertices[2].tex_coord.x );
    glVertex3dv( &vertices[2].position.x);
#endif

    glEnd();

    if ( materials_nonnull )
        vertices[0].material->reset_gl_state();
}

Intersect Sphere::intersect(Ray& ray) const
{
  Intersect itn;
  itn.intersect = false;


  Vector3 E = invMat.transform_point(ray.e);        // ray eye coordinate
  Vector3 D = invMat.transform_vector(ray.d);       // ray direction vector

  // calculate all the parameters
  real_t a = vertices[0].position.x - vertices[1].position.x;
  real_t b = vertices[0].position.y - vertices[1].position.y;
  real_t c = vertices[0].position.z - vertices[1].position.z;

  real_t d = vertices[0].position.x - vertices[2].position.x;
  real_t e = vertices[0].position.y - vertices[2].position.y;
  real_t f = vertices[0].position.z - vertices[2].position.z;

  real_t g = D.x;
  real_t h = D.y;
  real_t i = D.z;

  real_t j = vertices[0].position.x - E.x;
  real_t k = vertices[0].position.y - E.y;
  real_t l = vertices[0].position.z - E.z;

  // value cache
  real_t ei_hf = e * i - h * f;
  real_t gf_di = g * f - d * i;
  real_t dh_eg = d * h - e * g;

  real_t M = a * ei_hf + b * gf_di + c * dh_eg;

  // check t intersection
  real_t ak_jb = a * k - j * b;
  real_t jc_al = j * c - a * l;
  real_t bl_kc = b * l - k * c;
  real_t t = - (f * ak_jb + e * jc_al + d * bl_kc) / M;

  if (t < EPS)
    return itn;

  // check gamma intersection
  real_t gamma = (i * ak_jb + h * jc_al + g * bl_kc ) / M;
  if (gamma < 0.0 || gamma > 1.0)
    return itn;

  // check beta intersection
  real_t beta = (j * ei_hf + k * gf_di + l * dh_eg ) / M;
  if (beta < 0.0 || beta > 1.0 - gamma)
    return itn;

  // there is a intersection
  itn.intersect = true;

  // get normal transformation matrix
  Matrix4 invMat_inv;
  make_transformation_matrix(&invMat_inv, position, orientation, scale);

  // calcualte world position
  Vector3 local_pos = E + t * D;
  itn.position = invMat_inv.transform_point(local_pos);

  // calculate world normal
  Vector3 local_norm = normalize(vertices[0].normal + vertices[1].normal + vertices[2].normal);
  itn.normal = normalize(normMat * local_norm);

  // calculate ambient color
  itn.ambient = vertices[0].material->ambient * (1.0 - beta - gamma)
                + vertices[1].material->ambient * beta
                + vertices[2].material->ambient * gamma;

  // calculate diffuse color
  itn.diffuse = vertices[0].material->diffuse * (1.0 - beta - gamma)
                + vertices[1].material->diffuse * beta
                + vertices[2].material->diffuse * gamma;

  // calculate specular color
  itn.specular = vertices[0].material->specular * (1.0 - beta - gamma)
                 + vertices[1].material->specular * beta
                 + vertices[2].material->specular * gamma;

  // calulate refractive index
  itn.refractive_index = vertices[0].material->refractive_index * (1.0-beta-gamma)
                         + vertices[1].material->refractive_index * beta
                         + vertices[2].material->refractive_index * gamma;

  // calculate texture color at this point
  if (vertices[0].material->texture_filename.empty()
      && vertices[1].material->texture_filename.empty()
      && vertices[2].material->texture_filename.empty())
  {
    // if none of the material has texture, return white
    itn.texture = Color3::White();
  }
  else
  {
    // get texture coordinates
    Vector2 tex = vertices[0].tex_coord * (1.0 - beta - gamma)
                  + vertices[1].tex_coord * beta
                  + vertices[2].tex_coord * gamma;

    tex.x = tex.x - floor(tex.x);
    tex.y = tex.y - floor(tex.y);

    // get the width and height
    int width, height;

    // set color accumulator
    Color3 color = Color3::Black();

    vertices[0].material->get_texture_size(&width, &height);
    color += vertices[0].material->get_texture_pixel(
              (int)(tex.x*(real_t)(width)),
              (int)(tex.y*(real_t)(height))) * (1.0 - beta - gamma);

    vertices[1].material->get_texture_size(&width, &height);
    color += vertices[1].material->get_texture_pixel(
              (int)(tex.x*(real_t)(width)),
              (int)(tex.y*(real_t)(height))) * beta;

    vertices[2].material->get_texture_size(&width, &height);
    color += vertices[2].material->get_texture_pixel(
              (int)(tex.x*(real_t)(width)),
              (int)(tex.y*(real_t)(height)) ) * gamma;

    // set the final texture color
    itn.texture = color;
  }

  // return the value
  return itn;
}
} /* _462 */
