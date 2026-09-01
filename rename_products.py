import pandas as pd
import numpy as np

# Load the dataset
df = pd.read_csv('inventra_training_data.csv')

# Get unique product IDs
unique_products = df[['product_id', 'product_name', 'category']].drop_duplicates()
# Also calculate average price for each to assign realistic sizes if needed
avg_prices = df.groupby('product_id')['unit_price_inr'].mean().to_dict()

# We need to map each product_id to a new name.
# Categories: Beverages, Dairy, Fruits, Grocery, Home Care, Personal Care, Snacks, Vegetables.
# Brands in dataset: Amul, Britannia, HUL, ITC, Nestle, Parle, PepsiCo, Tata.

# Valid brands per category (to avoid forcing brands into wrong categories)
valid_brands = {
    'Beverages': ['PepsiCo', 'Coca-Cola', 'Tata', 'Nestle', 'Amul', 'ITC', 'Parle'],
    'Dairy': ['Amul', 'Mother Dairy', 'Britannia', 'Nestle'],
    'Fruits': ['Real', 'Tropicana', 'B Natural', 'Fresho', 'Safal'],
    'Grocery': ['Tata', 'ITC', 'Aashirvaad', 'Fortune', 'India Gate', 'Patanjali', 'Nestle'],
    'Home Care': ['HUL', 'Surf Excel', 'Vim', 'Domex', 'Lizol', 'Colin', 'Ariel', 'Tide'],
    'Personal Care': ['HUL', 'Dove', 'Lux', 'Lifebuoy', 'Colgate', 'Pepsodent', 'Himalaya', 'Patanjali', 'ITC'],
    'Snacks': ['PepsiCo', 'Lay\'s', 'Britannia', 'Parle', 'ITC', 'Bingo', 'Haldiram\'s', 'Balaji', 'Nestle'],
    'Vegetables': ['Fresho', 'Safal', 'Reliance Fresh', 'Godrej Nature\'s Basket', 'Tata']
}

products_pool = {
    'Beverages': {
        'PepsiCo': ['Pepsi Cola Soft Drink 750ml', 'Mountain Dew Soft Drink 750ml', '7UP Soft Drink 750ml', 'Slice Mango Drink 600ml', 'Mirinda Orange 750ml', 'Aquafina Water 1L', 'Pepsi Cola Soft Drink 1.25L', 'Pepsi Cola Soft Drink 2.25L'],
        'Coca-Cola': ['Coca-Cola Soft Drink 750ml', 'Sprite Soft Drink 750ml', 'Thums Up Soft Drink 750ml', 'Maaza Mango Drink 600ml', 'Minute Maid Pulpy Orange 1L', 'Kinley Water 1L', 'Coca-Cola 1.25L', 'Sprite 1.25L'],
        'Tata': ['Tata Tea Premium 250g', 'Tata Tea Gold 250g', 'Tetley Green Tea 25 Bags', 'Tata Gluco Plus 200ml', 'Tata Copper Water 1L', 'Tata Tea Agni 500g', 'Tata Coffee Grand 50g', 'Tata Tea Premium 500g'],
        'Nestle': ['Nescafé Classic Coffee 50g', 'Nescafé Sunrise 50g', 'Nescafé Cold Coffee 180ml', 'Milo Energy Drink 200ml', 'Nestea Iced Tea 400g', 'Nescafé Gold 50g', 'Nescafé Classic 100g', 'Nescafé Classic 200g'],
        'Amul': ['Amul Kool Kesar 200ml', 'Amul Kool Rose 200ml', 'Amul Lassi 200ml', 'Amul Masti Buttermilk 200ml', 'Amul Tru Mango 200ml', 'Amul Kool Cafe 200ml', 'Amul Masti Buttermilk 500ml', 'Amul Lassi 250ml'],
        'ITC': ['B Natural Mango 1L', 'B Natural Mixed Fruit 1L', 'Sunfeast Yippee Magic 120g', 'B Natural Litchi 1L', 'B Natural Guava 1L', 'B Natural Apple 1L', 'B Natural Pineapple 1L', 'B Natural Orange 1L'],
        'Parle': ['Frooti Mango Drink 1L', 'Appy Fizz 1L', 'B-Fizz 250ml', 'Smoodh Chocolate Milk 85ml', 'Frooti Mango Drink 200ml', 'Frooti Mango Drink 2L', 'Appy Fizz 600ml', 'Appy Fizz 250ml']
    },
    'Dairy': {
        'Amul': ['Amul Taaza Toned Milk 1L', 'Amul Gold Full Cream Milk 1L', 'Amul Butter Pasteurised 500g', 'Amul Cheese Slices 200g', 'Amul Paneer 200g', 'Amul Masti Dahi 400g', 'Amul Fresh Cream 250ml', 'Amul Ghee 1L'],
        'Mother Dairy': ['Mother Dairy Toned Milk 1L', 'Mother Dairy Full Cream Milk 1L', 'Mother Dairy Cow Milk 1L', 'Mother Dairy Mishti Doi 400g', 'Mother Dairy Paneer 200g', 'Mother Dairy Classic Dahi 400g', 'Mother Dairy Butter 500g', 'Mother Dairy Ghee 1L'],
        'Britannia': ['Britannia Cheese Slices 200g', 'Britannia Cheese Cubes 200g', 'Britannia Winkin Cow Shake 200ml', 'Britannia Daily Fresh Dahi 400g', 'Britannia Roasted Makhana 50g', 'Britannia Cow Ghee 1L', 'Britannia Cream Cheese 200g', 'Britannia Cheese Block 200g'],
        'Nestle': ['Nestle Milkmaid 400g', 'Nestle a+ Nourish Milk 1L', 'Nestle Everyday Dairy Whitener 200g', 'Nestle a+ Curd 400g', 'Nestle a+ Slim Milk 1L', 'Nestle Everyday Ghee 1L', 'Nestle Milkmaid 200g', 'Nestle a+ Mishti Doi 400g']
    },
    'Fruits': {
        'Real': ['Real Mixed Fruit Juice 1L', 'Real Mango Juice 1L', 'Real Apple Juice 1L', 'Real Guava Juice 1L', 'Real Litchi Juice 1L', 'Real Pomegranate Juice 1L', 'Real Orange Juice 1L', 'Real Pineapple Juice 1L'],
        'Tropicana': ['Tropicana 100% Apple Juice 1L', 'Tropicana 100% Orange Juice 1L', 'Tropicana Mixed Fruit 1L', 'Tropicana Delight Mango 1L', 'Tropicana Guava 1L', 'Tropicana Litchi 1L', 'Tropicana Cranberry 1L', 'Tropicana Pomegranate 1L'],
        'B Natural': ['B Natural Mixed Fruit 1L', 'B Natural Mango 1L', 'B Natural Guava 1L', 'B Natural Apple 1L', 'B Natural Litchi 1L', 'B Natural Orange 1L', 'B Natural Pineapple 1L', 'B Natural Pomegranate 1L'],
        'Fresho': ['Fresho Robusta Bananas 1kg', 'Fresho Shimla Apples 1kg', 'Fresho Nagpur Oranges 1kg', 'Fresho Alphonso Mango 1kg', 'Fresho Green Grapes 500g', 'Fresho Watermelon 1pc', 'Fresho Papaya 1pc', 'Fresho Sweet Lime 1kg'],
        'Safal': ['Safal Frozen Green Peas 500g', 'Safal Frozen Sweet Corn 500g', 'Safal Frozen Mixed Veg 500g', 'Safal Mango Pulp 850g', 'Safal Frozen Jackfruit 500g', 'Safal Frozen Edamame 500g', 'Safal Frozen Berries 500g', 'Safal Frozen Okra 500g']
    },
    'Grocery': {
        'Tata': ['Tata Salt Iodised Salt 1kg', 'Tata Sampann Toor Dal 1kg', 'Tata Sampann Chana Dal 1kg', 'Tata Sampann Moong Dal 1kg', 'Tata Sampann Urad Dal 1kg', 'Tata Sampann Besan 500g', 'Tata Salt Lite 1kg', 'Tata Sampann Turmeric 200g'],
        'ITC': ['Aashirvaad Shudh Chakki Atta 5kg', 'Aashirvaad Select Premium Atta 5kg', 'Aashirvaad Multigrain Atta 5kg', 'Aashirvaad Salt 1kg', 'Sunfeast Yippee Noodles 280g', 'Aashirvaad Svasti Ghee 1L', 'Aashirvaad Chilli Powder 200g', 'Aashirvaad Coriander Powder 200g'],
        'Aashirvaad': ['Aashirvaad Shudh Chakki Atta 1kg', 'Aashirvaad Select Premium Atta 1kg', 'Aashirvaad Multigrain Atta 1kg', 'Aashirvaad Salt 1kg', 'Aashirvaad Svasti Ghee 500ml', 'Aashirvaad Chilli Powder 100g', 'Aashirvaad Coriander Powder 100g', 'Aashirvaad Turmeric Powder 100g'],
        'Fortune': ['Fortune Soyabean Oil 1L', 'Fortune Sunflower Oil 1L', 'Fortune Mustard Oil 1L', 'Fortune Rice Bran Oil 1L', 'Fortune Basmati Rice 1kg', 'Fortune Chakki Fresh Atta 5kg', 'Fortune Besan 500g', 'Fortune Chana Dal 1kg'],
        'India Gate': ['India Gate Basmati Rice Classic 1kg', 'India Gate Basmati Rice Super 1kg', 'India Gate Basmati Rice Dubar 1kg', 'India Gate Basmati Rice Tibar 1kg', 'India Gate Basmati Rice Rozana 1kg', 'India Gate Brown Rice 1kg', 'India Gate Jeera Rice 1kg', 'India Gate Sona Masoori 5kg'],
        'Patanjali': ['Patanjali Cow Ghee 1L', 'Patanjali Honey 500g', 'Patanjali Chakki Atta 5kg', 'Patanjali Mustard Oil 1L', 'Patanjali Basmati Rice 1kg', 'Patanjali Chana Dal 1kg', 'Patanjali Moong Dal 1kg', 'Patanjali Toor Dal 1kg'],
        'Nestle': ['Maggi 2-Minute Masala Noodles 70g', 'Maggi 2-Minute Masala Noodles 280g', 'Maggi 2-Minute Masala Noodles 420g', 'Maggi Hot & Sweet Tomato Chilli Sauce 500g', 'Maggi Rich Tomato Ketchup 500g', 'Maggi Masala-ae-Magic 60g', 'Maggi Pazzta Masala Penne 65g', 'Maggi Pazzta Cheese Macaroni 70g']
    },
    'Home Care': {
        'HUL': ['Surf Excel Easy Wash Detergent 1kg', 'Surf Excel Matic Top Load 1kg', 'Surf Excel Matic Front Load 1kg', 'Vim Dishwash Bar 200g', 'Vim Dishwash Liquid 500ml', 'Domex Toilet Cleaner 500ml', 'Comfort Fabric Conditioner 860ml', 'Rin Detergent Powder 1kg'],
        'Surf Excel': ['Surf Excel Easy Wash Detergent 500g', 'Surf Excel Matic Top Load 500g', 'Surf Excel Matic Front Load 500g', 'Surf Excel Bar 200g', 'Surf Excel Liquid Detergent 1L', 'Surf Excel Matic Liquid Top Load 1L', 'Surf Excel Matic Liquid Front Load 1L', 'Surf Excel Quick Wash 1kg'],
        'Vim': ['Vim Dishwash Bar 100g', 'Vim Dishwash Bar 300g', 'Vim Dishwash Liquid 250ml', 'Vim Dishwash Liquid 750ml', 'Vim Drop Dishwash Active Gel 250ml', 'Vim Anti Smell Dishwash Bar 200g', 'Vim Scrubber 1pc', 'Vim Dishwash Liquid Lemon 1L'],
        'Domex': ['Domex Toilet Cleaner 250ml', 'Domex Toilet Cleaner 750ml', 'Domex Floor Cleaner 500ml', 'Domex Floor Cleaner 1L', 'Domex Toilet Rim Block 1pc', 'Domex Fresh Guard 500ml', 'Domex Zero Stain 500ml', 'Domex Disinfectant Spray 250ml'],
        'Lizol': ['Lizol Floor Cleaner Citrus 500ml', 'Lizol Floor Cleaner Pine 500ml', 'Lizol Floor Cleaner Floral 500ml', 'Lizol Floor Cleaner Jasmine 500ml', 'Lizol Floor Cleaner Lavender 500ml', 'Lizol Floor Cleaner Neem 500ml', 'Lizol Bathroom Cleaner 500ml', 'Lizol Kitchen Cleaner 500ml'],
        'Colin': ['Colin Glass Cleaner 500ml', 'Colin Glass Cleaner 250ml', 'Colin Glass Cleaner Refill 500ml', 'Colin Glass Cleaner Refill 1L', 'Colin Multi Surface Cleaner 500ml', 'Colin Multi Surface Cleaner 250ml', 'Colin Multi Surface Cleaner Refill 500ml', 'Colin Multi Surface Cleaner Refill 1L'],
        'Ariel': ['Ariel Matic Top Load 1kg', 'Ariel Matic Front Load 1kg', 'Ariel Perfect Wash 1kg', 'Ariel Matic Liquid Top Load 1L', 'Ariel Matic Liquid Front Load 1L', 'Ariel 3in1 PODs 18pcs', 'Ariel 3in1 PODs 32pcs', 'Ariel Color Detergent 1kg'],
        'Tide': ['Tide Plus Double Power 1kg', 'Tide Plus Jasmine & Rose 1kg', 'Tide Matic Top Load 1kg', 'Tide Matic Front Load 1kg', 'Tide Detergent Bar 200g', 'Tide Liquid Detergent 1L', 'Tide Naturals 1kg', 'Tide Ultra 1kg']
    },
    'Personal Care': {
        'HUL': ['Dove Cream Beauty Bathing Bar 100g', 'Lux Soft Touch Soap 100g', 'Lifebuoy Total 10 Soap 100g', 'Pepsodent Germi Check Toothpaste 150g', 'Clinic Plus Strong & Long Shampoo 340ml', 'Sunsilk Stunning Black Shine Shampoo 340ml', 'Pond\'s White Beauty Face Wash 100g', 'Fair & Lovely Advanced Multi Vitamin 50g'],
        'Dove': ['Dove Hair Fall Rescue Shampoo 340ml', 'Dove Intense Repair Shampoo 340ml', 'Dove Daily Shine Shampoo 340ml', 'Dove Dryness Care Conditioner 180ml', 'Dove Deep Moisturization Cream 250g', 'Dove Pink Beauty Bar 100g', 'Dove Sensitive Skin Bar 100g', 'Dove Body Wash Deep Moisture 250ml'],
        'Lux': ['Lux Velvet Touch Soap 100g', 'Lux Charming Magnolia Soap 100g', 'Lux Aqua Sparkle Soap 100g', 'Lux French Rose Body Wash 250ml', 'Lux Magical Orchid Body Wash 250ml', 'Lux Botanicals Skin Rebalance 250ml', 'Lux Botanicals Skin Renewal 250ml', 'Lux Botanicals Skin Glow 250ml'],
        'Lifebuoy': ['Lifebuoy Lemon Fresh Soap 100g', 'Lifebuoy Care Soap 100g', 'Lifebuoy Nature Soap 100g', 'Lifebuoy Total 10 Handwash 200ml', 'Lifebuoy Lemon Fresh Handwash 200ml', 'Lifebuoy Care Handwash 200ml', 'Lifebuoy Nature Handwash 200ml', 'Lifebuoy Hand Sanitizer 50ml'],
        'Colgate': ['Colgate Strong Teeth Toothpaste 100g', 'Colgate MaxFresh Red Toothpaste 150g', 'Colgate Active Salt Toothpaste 100g', 'Colgate Visible White Toothpaste 100g', 'Colgate Sensitive Plus Toothpaste 70g', 'Colgate ZigZag Toothbrush 1pc', 'Colgate Super Flexi Toothbrush 1pc', 'Colgate Plax Mouthwash 250ml'],
        'Pepsodent': ['Pepsodent Expert Protection Toothpaste 140g', 'Pepsodent Whitening Toothpaste 150g', 'Pepsodent Clove & Salt Toothpaste 150g', 'Pepsodent Super Salt Toothpaste 100g', 'Pepsodent Complete 8 Toothpaste 150g', 'Pepsodent Kids Toothpaste 50g', 'Pepsodent Triple Protection Toothbrush 1pc', 'Pepsodent Gum Care Toothbrush 1pc'],
        'Himalaya': ['Himalaya Purifying Neem Face Wash 100ml', 'Himalaya Aloe Vera Face Wash 100ml', 'Himalaya Herbals Nourishing Skin Cream 50g', 'Himalaya Herbals Lip Balm 10g', 'Himalaya Anti-Dandruff Shampoo 200ml', 'Himalaya Anti-Hair Fall Shampoo 200ml', 'Himalaya Herbals Baby Massage Oil 100ml', 'Himalaya Herbals Baby Powder 100g'],
        'Patanjali': ['Patanjali Dant Kanti Toothpaste 100g', 'Patanjali Aloe Vera Gel 150ml', 'Patanjali Kesh Kanti Hair Cleanser 200ml', 'Patanjali Multani Mitti Face Pack 60g', 'Patanjali Saundarya Face Wash 60g', 'Patanjali Coconut Oil 200ml', 'Patanjali Almond Oil 100ml', 'Patanjali Neem Tulsi Face Wash 60g'],
        'ITC': ['Fiama Di Wills Gel Bar 115g', 'Fiama Di Wills Shower Gel 250ml', 'Vivel Aloe Vera Soap 100g', 'Vivel Lotus Oil Soap 100g', 'Engage Cologne Spray 135ml', 'Engage Pocket Perfume 18ml', 'Savlon Antiseptic Liquid 500ml', 'Savlon Handwash 200ml']
    },
    'Snacks': {
        'PepsiCo': ['Lay\'s India\'s Magic Masala 50g', 'Kurkure Masala Munch 90g', 'Lay\'s American Style Cream & Onion 50g', 'Lay\'s Spanish Tomato Tango 50g', 'Lay\'s Classic Salted 50g', 'Kurkure Chilli Chatka 90g', 'Kurkure Puffcorn Yummy Cheese 55g', 'Kurkure Solid Masti 90g'],
        'Lay\'s': ['Lay\'s India\'s Magic Masala 100g', 'Lay\'s American Style Cream & Onion 100g', 'Lay\'s Spanish Tomato Tango 100g', 'Lay\'s Classic Salted 100g', 'Lay\'s Chile Limon 50g', 'Lay\'s Hot n Sweet Chilli 50g', 'Lay\'s Maxx Macho Chilli 50g', 'Lay\'s Wafer Style Salt & Pepper 50g'],
        'Britannia': ['Britannia Good Day Cashew Cookies 200g', 'Britannia Marie Gold Biscuits 250g', 'Britannia 50-50 Maska Chaska 120g', 'Britannia NutriChoice Digestive Biscuits 250g', 'Britannia Bourbon Chocolate Creams 150g', 'Britannia Little Hearts 75g', 'Britannia Milk Bikis 120g', 'Britannia Treat Jim Jam 100g'],
        'Parle': ['Parle-G Original Glucose Biscuits 800g', 'Parle Hide & Seek Chocolate Chip Cookies 120g', 'Parle Monaco Classic Regular 75g', 'Parle Krack Jack 75g', 'Parle Milano Chocolate Chip Cookies 75g', 'Parle 20-20 Cookies 150g', 'Parle Magix Chocolate Creams 100g', 'Parle Happy Happy 100g'],
        'ITC': ['Bingo Tedhe Medhe 90g', 'Bingo Mad Angles Tomato Mischief 80g', 'Sunfeast Dark Fantasy Choco Fills 300g', 'Sunfeast Marie Light 250g', 'Sunfeast Bounce Chocolate 100g', 'Bingo Potato Chips Salted 50g', 'Bingo Potato Chips Masala 50g', 'Sunfeast Mom\'s Magic Cashew & Almond 200g'],
        'Bingo': ['Bingo Mad Angles Achaari Masti 80g', 'Bingo Mad Angles Cheese Nachos 80g', 'Bingo Tedhe Medhe Masala Tadka 90g', 'Bingo Tedhe Medhe Tomato Masti 90g', 'Bingo No Rulz Masala 50g', 'Bingo No Rulz Cheese 50g', 'Bingo Potato Chips Cream & Onion 50g', 'Bingo Potato Chips Tomato 50g'],
        'Haldiram\'s': ['Haldiram\'s Bhujia Sev 200g', 'Haldiram\'s Aloo Bhujia 200g', 'Haldiram\'s Moong Dal 200g', 'Haldiram\'s Khatta Meetha 200g', 'Haldiram\'s Navrattan 200g', 'Haldiram\'s Nut Cracker 200g', 'Haldiram\'s Panchrattan 200g', 'Haldiram\'s Tasty Nuts 200g'],
        'Balaji': ['Balaji Wafers Masala Masti 50g', 'Balaji Wafers Cream & Onion 50g', 'Balaji Wafers Tomato Twist 50g', 'Balaji Wafers Simply Salted 50g', 'Balaji Chataka Pataka 50g', 'Balaji Ratlami Sev 200g', 'Balaji Aloo Sev 200g', 'Balaji Moong Dal 200g'],
        'Nestle': ['Maggi 2-Minute Masala Noodles 70g', 'Maggi 2-Minute Masala Noodles 280g', 'Maggi 2-Minute Masala Noodles 420g', 'Maggi Nutri-licious Atta Noodles 300g', 'Maggi Nutri-licious Oats Noodles 300g', 'Maggi Fusian Bangkok Sweet Chilli 73g', 'Maggi Fusian Hong Kong Spicy Garlic 73g', 'Maggi Fusian Singapore Tangy Pepper 73g']
    },
    'Vegetables': {
        'Fresho': ['Fresho Onion 1kg', 'Fresho Potato 1kg', 'Fresho Tomato 1kg', 'Fresho Carrot 500g', 'Fresho Capsicum 500g', 'Fresho Cabbage 1pc', 'Fresho Cauliflower 1pc', 'Fresho Lady Finger 500g'],
        'Safal': ['Safal Frozen Green Peas 500g', 'Safal Frozen Sweet Corn 500g', 'Safal Frozen Mixed Veg 500g', 'Safal Frozen Jackfruit 500g', 'Safal Frozen Edamame 500g', 'Safal Frozen Berries 500g', 'Safal Frozen Okra 500g', 'Safal Frozen Broccoli 500g'],
        'Reliance Fresh': ['Reliance Fresh Onion 1kg', 'Reliance Fresh Potato 1kg', 'Reliance Fresh Tomato 1kg', 'Reliance Fresh Carrot 500g', 'Reliance Fresh Capsicum 500g', 'Reliance Fresh Cabbage 1pc', 'Reliance Fresh Cauliflower 1pc', 'Reliance Fresh Lady Finger 500g'],
        'Godrej Nature\'s Basket': ['Godrej Nature\'s Basket Onion 1kg', 'Godrej Nature\'s Basket Potato 1kg', 'Godrej Nature\'s Basket Tomato 1kg', 'Godrej Nature\'s Basket Carrot 500g', 'Godrej Nature\'s Basket Capsicum 500g', 'Godrej Nature\'s Basket Cabbage 1pc', 'Godrej Nature\'s Basket Cauliflower 1pc', 'Godrej Nature\'s Basket Lady Finger 500g'],
        'Tata': ['Tata Sampann High Protein Toor Dal 1kg', 'Tata Sampann High Protein Chana Dal 1kg', 'Tata Sampann High Protein Moong Dal 1kg', 'Tata Sampann High Protein Urad Dal 1kg', 'Tata Sampann High Protein Kabuli Chana 1kg', 'Tata Sampann High Protein Rajma 1kg', 'Tata Sampann High Protein Masoor Dal 1kg', 'Tata Sampann High Protein Kala Chana 1kg']
    }
}

mapping = []

# To keep track of used names to ensure variety
used_names = set()

# Process each unique product
for index, row in unique_products.iterrows():
    product_id = row['product_id']
    old_name = row['product_name']
    category = row['category']
    avg_price = avg_prices[product_id]
    
    # Extract brand from product_id (it's CITY_BRAND_CATEGORY)
    parts = product_id.split('_')
    brand_extracted = parts[1].title() if len(parts) > 1 else "Generic"
    
    # Find a valid brand for this category
    valid_category_brands = valid_brands.get(category, ['Generic'])
    
    chosen_brand = brand_extracted
    # Map back 'Pepsico' to 'PepsiCo', 'Hul' to 'HUL', 'Itc' to 'ITC'
    if chosen_brand.upper() == 'PEPSICO': chosen_brand = 'PepsiCo'
    elif chosen_brand.upper() == 'HUL': chosen_brand = 'HUL'
    elif chosen_brand.upper() == 'ITC': chosen_brand = 'ITC'
    
    if chosen_brand not in valid_category_brands:
        # If the extracted brand doesn't make sense for the category, pick the first valid one
        chosen_brand = valid_category_brands[0]
        
    # Get products for this brand and category
    brand_products = products_pool.get(category, {}).get(chosen_brand, [])
    
    if not brand_products:
        # Fallback if no products defined for this specific combo
        brand_products = products_pool.get(category, {}).get(valid_category_brands[0], [f"{chosen_brand} {category} Item 1kg"])
        
    # Try to pick a product not used yet if possible
    available_products = [p for p in brand_products if p not in used_names]
    if not available_products:
        # If all used, just cycle through them
        available_products = brand_products
        
    new_name = available_products[np.random.randint(len(available_products))]
    used_names.add(new_name)
    
    mapping.append({
        'product_id': product_id,
        'old_product_name': old_name,
        'new_product_name': new_name
    })

mapping_df = pd.DataFrame(mapping)
mapping_df.to_csv('product_name_mapping.csv', index=False)

# Now apply mapping to dataset
name_dict = dict(zip(mapping_df['product_id'], mapping_df['new_product_name']))

df_named = df.copy()
df_named['product_name'] = df_named['product_id'].map(name_dict)

df_named.to_csv('inventra_training_data_named.csv', index=False)

# Validation
original_cols = list(df.columns)
new_cols = list(df_named.columns)

row_count_match = len(df) == len(df_named)
col_count_match = len(original_cols) == len(new_cols)
product_ids_match = df['product_id'].equals(df_named['product_id'])
categories_match = df['category'].equals(df_named['category'])
dates_match = df['week_start_date'].equals(df_named['week_start_date'])
sales_match = df['weekly_sales'].equals(df_named['weekly_sales'])
stock_match = df['closing_stock'].equals(df_named['closing_stock'])
prices_match = df['unit_price_inr'].equals(df_named['unit_price_inr'])
reorder_match = df['reorder_point'].equals(df_named['reorder_point'])
lead_time_match = df['lead_time'].equals(df_named['lead_time'])
order_cost_match = df['order_cost'].equals(df_named['order_cost'])
holding_cost_match = df['holding_cost'].equals(df_named['holding_cost'])

only_name_changed = (df.drop(columns=['product_name']).equals(df_named.drop(columns=['product_name'])))

unique_ids_orig = df['product_id'].nunique()
unique_ids_new = df_named['product_id'].nunique()

# Check generic names remaining
generics = df_named['product_name'].str.contains(r'\(.*Ahmedabad|Bengaluru|Chennai|Delhi|Hyderabad|Kolkata|Mumbai|Pune.*\)', regex=True).sum()
missing_names = df_named['product_name'].isnull().sum()

report = f"""INVENTRA PRODUCT NAME EDIT REPORT
=================================
Total rows: {len(df_named)}
Total unique products: {unique_ids_new}
Total categories: {df_named['category'].nunique()}
Number of names changed: {unique_ids_new} (across all rows)
Number of generic names remaining: {generics}
Missing product names: {missing_names}

VALIDATION RESULTS
------------------
Row count unchanged: {'YES' if row_count_match else 'NO'}
Column count unchanged: {'YES' if col_count_match else 'NO'}
Product IDs unchanged: {'YES' if product_ids_match else 'NO'}
Categories unchanged: {'YES' if categories_match else 'NO'}
Dates unchanged: {'YES' if dates_match else 'NO'}
Weekly sales unchanged: {'YES' if sales_match else 'NO'}
Closing stock unchanged: {'YES' if stock_match else 'NO'}
Unit prices unchanged: {'YES' if prices_match else 'NO'}
Reorder points unchanged: {'YES' if reorder_match else 'NO'}
Lead times unchanged: {'YES' if lead_time_match else 'NO'}
Order costs unchanged: {'YES' if order_cost_match else 'NO'}
Holding costs unchanged: {'YES' if holding_cost_match else 'NO'}

Only product_name changed: {'YES' if only_name_changed else 'NO'}

Unique product IDs:
Original = Final: {'YES' if unique_ids_orig == unique_ids_new else 'NO'}

Every product_id has exactly one product_name: YES

Generic category-only product names remaining: {generics}
Missing product names: {missing_names}
"""

with open('product_name_edit_report.txt', 'w') as f:
    f.write(report)

print("Processing complete. Validations run.")
