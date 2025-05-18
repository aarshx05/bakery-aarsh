// src/data/products.ts

import { ProductType, CategoryType } from '../types/types';

export const categories: CategoryType[] = [
  {
    id: '1',
    name: 'Sweet Bakes',
    slug: 'sweet-bakes',
    image: 'assets/sb.jpg',
  },
  {
    id: '2',
    name: 'Celebration Cakes',
    slug: 'celebration-cakes',
    image: 'assets/bc.jpg',
  },
  {
    id: '3',
    name: 'Savouries & Buns',
    slug: 'savouries-buns',
    image: 'assets/ss.jpg',
  },
];

export const products: ProductType[] = [
  // SWEET BAKES
  {
    id: 'fruit-and-nut-300-gms',
    name: 'Fruit & Nut (300 GMS)',
    description:
      'Vanilla pound cake with cashews, almonds, cranberries, golden raisins, and apricots. CONTAINS NUTS & EGGS',
    price: 650,
    image: 'assets/fn.svg',
    category: 'sweet-bakes',
    featured: true,
  },
  {
    id: 'carrot-and-ginger-450-gms',
    name: 'Carrot & Ginger (450 GMS)',
    description:
      'Carrot cake with a gingery bite and cream cheese or cinnamon frosting. CONTAINS EGGS',
    price: 650,
    image: 'assets/cc.svg',
    category: 'sweet-bakes',
  },
  {
    id: 'chocolate-orange-300-gms',
    name: '54.5% & Orange (300 GMS)',
    description:
      'Belgian dark chocolate with candied orange bits and zest. CONTAINS EGGS',
    price: 650,
    image: 'assets/oc.svg',
    category: 'sweet-bakes',
  },
  {
    id: 'classic-choc-chip-pack-3',
    name: 'Classic Choc Chip Cookies (Pack of 3)',
    description:
      'Belgian chocolate callets in a classic cookie dough. CONTAINS EGGS',
    price: 480,
    image: 'assets/chc.svg',
    category: 'sweet-bakes',
  },

  // CELEBRATION CAKES (1/2 KG & 1 KG VARIANTS)
  {
    id: 'belgian-chocolate-og-half-kg',
    name: 'The OG Belgian Chocolate (1/2 KG)',
    description:
      'Chocolate cake with ganache from 70% Belgian dark & milk chocolate.',
    price: 1150,
    image: 'assets/bc.jpg',
    category: 'celebration-cakes',
    featured: true,
  },
  {
    id: 'belgian-chocolate-og-1kg',
    name: 'The OG Belgian Chocolate (1 KG)',
    description:
      'Chocolate cake with ganache from 70% Belgian dark & milk chocolate.',
    price: 2050,
    image: 'assets/bc.jpg',
    category: 'celebration-cakes',
  },
  {
    id: 'pineapples-and-cream-half-kg',
    name: 'Pineapples & Cream (1/2 KG)',
    description: 'Fresh pineapples with Vanilla Chantilly.',
    price: 850,
    image: 'assets/pc.svg',
    category: 'celebration-cakes',
  },
  {
    id: 'pineapples-and-cream-1kg',
    name: 'Pineapples & Cream (1 KG)',
    description: 'Fresh pineapples with Vanilla Chantilly.',
    price: 1550,
    image: 'assets/pc.svg',
    category: 'celebration-cakes',
  },
  {
    id: 'snickers-half-kg',
    name: 'Snickers Cake (1/2 KG)',
    description:
      'Roasted peanuts, praline, salted caramel, chocolate chantilly & cake.',
    price: 1050,
    image: 'https://images.pexels.com/photos/14105/pexels-photo.jpg',
    category: 'celebration-cakes',
    featured: true,
  },
  {
    id: 'snickers-1kg',
    name: 'Snickers Cake (1 KG)',
    description:
      'Roasted peanuts, praline, salted caramel, chocolate chantilly & cake.',
    price: 1850,
    image: 'https://images.pexels.com/photos/14105/pexels-photo.jpg',
    category: 'celebration-cakes',
  },
  {
    id: 'gluten-free-chocolate-cake',
    name: 'Gluten-free Chocolate Cake',
    description: 'A decadent brownie-like moist chocolate cake. GLUTEN-FREE',
    price: 1050,
    image: 'https://images.pexels.com/photos/291528/pexels-photo-291528.jpeg',
    category: 'celebration-cakes',
    featured: true,
  },

  // SAVOURIES & BUNS (Pack variants)
  {
    id: 'og-shio-pan-pack-6',
    name: 'The OG Shio Pan (Pack of 6)',
    description: 'Fluffy crescent bun with butter and crispy base.',
    price: 450,
    image: 'https://images.pexels.com/photos/209318/pexels-photo-209318.jpeg',
    category: 'savouries-buns',
  },
  {
    id: 'og-shio-pan-pack-10',
    name: 'The OG Shio Pan (Pack of 10)',
    description: 'Fluffy crescent bun with butter and crispy base.',
    price: 600,
    image: 'https://images.pexels.com/photos/209318/pexels-photo-209318.jpeg',
    category: 'savouries-buns',
  },
  {
    id: 'thecha-shio-pan-pack-6',
    name: 'Thecha Shio Pan (Pack of 6)',
    description: 'Fluffy bun with chillies, coriander, and garlic.',
    price: 475,
    image: 'https://images.pexels.com/photos/209318/pexels-photo-209318.jpeg',
    category: 'savouries-buns',
  },
  {
    id: 'thecha-shio-pan-pack-10',
    name: 'Thecha Shio Pan (Pack of 10)',
    description: 'Fluffy bun with chillies, coriander, and garlic.',
    price: 625,
    image: 'https://images.pexels.com/photos/209318/pexels-photo-209318.jpeg',
    category: 'savouries-buns',
  },
  {
    id: 'korean-garlic-shio-pan-pack-6',
    name: 'Korean Garlic Shio Pan (Pack of 6)',
    description: 'Salted crescent buns filled with herbed garlic cream cheese.',
    price: 500,
    image: 'https://images.pexels.com/photos/209318/pexels-photo-209318.jpeg',
    category: 'savouries-buns',
    featured: true,
  },
  {
    id: 'korean-garlic-shio-pan-pack-10',
    name: 'Korean Garlic Shio Pan (Pack of 10)',
    description: 'Salted crescent buns filled with herbed garlic cream cheese.',
    price: 650,
    image: 'https://images.pexels.com/photos/209318/pexels-photo-209318.jpeg',
    category: 'savouries-buns',
  },
  {
    id: 'jalapeno-cheddar-shio-pan-pack-6',
    name: 'Jalapeño & Cheddar Shio Pan (Pack of 6)',
    description: 'Spicy jalapeño and cheddar buns with buttery goodness.',
    price: 650,
    image: 'https://images.pexels.com/photos/209318/pexels-photo-209318.jpeg',
    category: 'savouries-buns',
  },
  {
    id: 'jalapeno-cheddar-shio-pan-pack-10',
    name: 'Jalapeño & Cheddar Shio Pan (Pack of 10)',
    description: 'Spicy jalapeño and cheddar buns with buttery goodness.',
    price: 850,
    image: 'https://images.pexels.com/photos/209318/pexels-photo-209318.jpeg',
    category: 'savouries-buns',
  },
];

export const getFeaturedProducts = (): ProductType[] => {
  return products.filter((product) => product.featured);
};

export const getProductsByCategory = (category: string): ProductType[] => {
  return products.filter((product) => product.category === category);
};

export const getProductById = (id: string): ProductType | undefined => {
  return products.find((product) => product.id === id);
};
